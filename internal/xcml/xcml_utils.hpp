#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <fmt/format.h>
#include <inttypes.h>
#include <pugixml.hpp>
#include <xcml.hpp>

namespace xcml {

inline pugi::xml_document read_xml(std::string const& input) {
    pugi::xml_document doc;

    std::ifstream ifs;
    std::reference_wrapper<std::istream> is = std::cin;
    if (!(input.empty() || input == "-")) {
        ifs.open(input);
        is = ifs;
    }

    if (auto const result = doc.load(is); !result) {
        fmt::print(stderr, "XML Error: {}\n", result.description());
        return {};
    }

    return doc;
}

inline void write_xml(std::string const& output, pugi::xml_document const& doc) {
    if (output.empty() || output == "-") {
        doc.save(std::cout, "  ");
    } else {
        std::ofstream ofs(output);
        doc.save(ofs, "  ");
    }
}

inline xcml_program_node_ptr xml_to_prg(pugi::xml_document const& doc) {
    auto prg = new_xcml_program_node();
    xcml::from_xml(*doc.begin(), prg);
    return prg;
}

inline pugi::xml_document prg_to_xml(xcml_program_node_ptr prg) {
    pugi::xml_document doc;
    xcml::to_xml(doc, prg);
    return doc;
}

inline void clear_extra(xcml_program_node_ptr prg) {
    prg->extra.clear();
}

compound_stmt_ptr clone_compound(compound_stmt_ptr const& node);

expr_ptr copy_expr_impl(xcml_program_node_ptr const& prg, expr_ptr const& node);

template <class Expr>
inline std::shared_ptr<Expr> copy_expr(xcml_program_node_ptr const& prg,
                                       std::shared_ptr<Expr> const& node) {
    static_assert(std::is_base_of_v<xcml::expr_node, Expr>);

    return Expr::dyncast(copy_expr_impl(prg, node));
}

inline void dump_node(std::shared_ptr<node> n) {
    pugi::xml_document doc;
    xcml::to_xml(doc, n);
    doc.save(std::cerr, "", pugi::format_raw);
    std::cerr << std::endl;
}

}  // namespace xcml

namespace xcml::utils {

[[noreturn]] inline void type_not_found(std::string_view name) {
    auto const errmsg = fmt::format("Type not found: {}", name);
    throw std::runtime_error(errmsg);
}

[[noreturn]] inline void not_struct(std::string_view name) {
    auto const errmsg = fmt::format("Not struct type: {}", name);
    throw std::runtime_error(errmsg);
}

[[noreturn]] inline void not_array(std::string_view name) {
    auto const errmsg = fmt::format("Not array type: {}", name);
    throw std::runtime_error(errmsg);
}

[[noreturn]] inline void not_function(std::string_view name) {
    auto const errmsg = fmt::format("Not function type: {}", name);
    throw std::runtime_error(errmsg);
}

[[noreturn]] inline void symbol_not_found(xcml::type_ptr const& type, std::string_view name) {
    auto const errmsg = fmt::format("symbol not found: {} in {}", name, type->type);
    throw std::runtime_error(errmsg);
}

[[noreturn]] inline void param_not_found(xcml::function_type_ptr const& type,
                                         std::string_view name) {
    auto const errmsg =
        fmt::format("symbol not found in the parameters: {} in {}", name, type->type);
    throw std::runtime_error(errmsg);
}

[[noreturn]] inline void not_supported_type(std::string_view fn, std::string_view type) {
    auto const errmsg = fmt::format("type not supported: {} by {}", type, fn);
    throw std::runtime_error(errmsg);
}

inline bool is_singed_int_type(std::string_view type) {
    return type == "char" || type == "short" || type == "int" || type == "long" ||
           type == "long_long";
}

inline bool is_unsinged_int_type(std::string_view type) {
    return type == "_Bool" || type == "unsigned_char" || type == "unsigned_short" ||
           type == "unsigned" || type == "unsigned_long" || type == "unsigned_long_long";
}

inline bool is_integral_type(std::string_view type) {
    return is_singed_int_type(type) || is_unsinged_int_type(type);
}

inline bool is_builtin_type(std::string_view type) {
    return type == "void" || is_integral_type(type) || type == "float" || type == "double" ||
           type == "long_double";
}

inline auto add_sym(xcml::xcml_program_node_ptr prg, std::string_view type,
                    xcml::storage_class sclass, std::string_view name) {
    auto sym = new_symbol_id();
    sym->sclass = sclass;
    sym->type = type;
    sym->name = name;

    prg->global_symbols.push_back(sym);
    return sym;
}

inline auto make_expr_stmt(xcml::expr_ptr expr) {
    auto stmt = new_expr_stmt();
    stmt->expr = expr;
    return stmt;
}

inline void push_stmt(xcml::compound_stmt_ptr const& compound, xcml::stmt_ptr const& stmt) {
    if (stmt) {
        compound->body.push_back(stmt);
    }
}

inline void push_expr(xcml::compound_stmt_ptr const& compound, xcml::expr_ptr const& expr) {
    if (expr) {
        push_stmt(compound, make_expr_stmt(expr));
    }
}

inline void prepend_expr(xcml::compound_stmt_ptr const& compound, xcml::expr_ptr const& expr) {
    if (expr) {
        compound->body.push_front(make_expr_stmt(expr));
    }
}

inline void sort_decls(xcml::xcml_program_node_ptr const& node) {
    auto it = node->global_declarations.begin();

    it = std::stable_partition(it, node->global_declarations.end(), [](auto const& node) {
        return xcml::cpp_include::is_a(node);
    });

    it = std::stable_partition(it, node->global_declarations.end(), [](auto const& node) {
        return xcml::function_decl::is_a(node) || xcml::runtime_func_decl::is_a(node);
    });
}

struct fdecl_opts {
    bool is_static = false;
    bool extern_c = false;
    bool is_inline = false;
    bool is_force_inline = false;
    bool no_add_sym = false;

    static fdecl_opts static_() {
        fdecl_opts opts;
        opts.is_static = true;
        return opts;
    }
};

inline xcml::func_addr_ptr add_fdecl(xcml::xcml_program_node_ptr prg,
                                     xcml::function_type_ptr const& ft, std::string_view name,
                                     fdecl_opts const& opts) {
    auto decl = new_function_decl();
    decl->name = name;
    decl->extern_c = opts.extern_c;
    decl->inline_ = opts.is_inline;
    decl->force_inline = opts.is_force_inline;

    prg->global_declarations.push_back(decl);

    if (!opts.no_add_sym) {
        auto const sym = new_symbol_id();
        sym->name = name;
        sym->type = ft->type;
        sym->sclass = opts.is_static ? storage_class::static_ : storage_class::extern_def;

        prg->global_symbols.push_back(sym);
    }

    auto addr = new_func_addr();
    addr->name = name;
    return addr;
}

inline xcml::function_definition_ptr make_fd(xcml::xcml_program_node_ptr prg,
                                             xcml::function_type_ptr const& ft,
                                             std::string_view name, bool no_add = false) {
    auto fd = new_function_definition();
    fd->name = name;
    fd->params = ft->params;
    for (auto p : fd->params) {
        if (auto param = xcml::param_node::dyncast(p)) {
            auto sym = new_symbol_id();
            sym->name = param->name;
            sym->sclass = xcml::storage_class::param;
            sym->type = param->type;
            fd->symbols.push_back(sym);
        }
    }

    if (!no_add) {
        prg->global_declarations.push_back(fd);
    }

    return fd;
}

inline xcml::var_ref_ptr param_ref(xcml::function_definition_ptr& fd, std::string_view name) {
    for (auto p : fd->params) {
        if (auto param = xcml::param_node::dyncast(p)) {
            if (param->name == name) {
                auto ref = new_var_ref();
                ref->name = param->name;
                ref->scope = xcml::ref_scope::param;
                return ref;
            }
        }
    }
    return nullptr;
}

inline std::string param_type(xcml::function_definition_ptr& fd, std::string_view name) {
    for (auto p : fd->params) {
        if (auto param = xcml::param_node::dyncast(p)) {
            if (param->name == name) {
                return param->type;
            }
        }
    }
    return nullptr;
}

inline xcml::type_ptr get_type(xcml::xcml_program_node_ptr prg, std::string_view name) {
    for (auto t : prg->type_table) {
        if (t->type == name) {
            return t;
        }
    }
    type_not_found(name);
}

inline xcml::function_type_ptr get_function_type(xcml::xcml_program_node_ptr prg,
                                                 std::string_view name) {
    auto ft = xcml::function_type::dyncast(get_type(prg, name));
    if (!ft) {
        not_function(name);
    }
    return ft;
}

inline xcml::function_definition_ptr get_function_definition(xcml::xcml_program_node_ptr prg,
                                                             std::string_view name) {
    for (auto d : prg->global_declarations) {
        if (auto fd = function_definition::dyncast(d); fd && fd->name == name) {
            return fd;
        }
    }

    not_function(name);
}

inline xcml::type_ptr get_pointee_type(xcml::xcml_program_node_ptr prg,
                                       xcml::pointer_type_ptr p) {
    auto name = p->ref;

    for (;;) {
        auto t = get_type(prg, name);

        if (auto bt = xcml::basic_type::dyncast(t)) {
            if (std::islower(bt->name[0])) {
                return bt;
            }

            name = bt->name;
        } else {
            return t;
        }
    }
}

inline xcml::struct_type_ptr get_struct_type(xcml::xcml_program_node_ptr prg,
                                             std::string_view name) {
    auto t = get_type(prg, name);

    if (auto p = xcml::pointer_type::dyncast(t)) {
        t = get_pointee_type(prg, p);
    }
    if (auto s = xcml::struct_type::dyncast(t)) {
        return s;
    }
    not_struct(name);
}

template <class Node>
inline xcml::symbol_id_ptr get_symbol(Node& node, std::string_view name) {
    for (auto sym : node->symbols) {
        if (sym->name == name) {
            return sym;
        }
    }
    symbol_not_found(node, name);
}

inline xcml::member_ref_ptr make_member_ref(xcml::expr_ptr const& ref, std::string_view name) {
    auto mref = new_member_ref();
    mref->member = name;
    mref->value = ref;
    return mref;
}

inline xcml::expr_ptr make_addr_of(xcml::expr_ptr const& expr) {
    auto addr = new_addr_of_expr();
    addr->expr = expr;
    return addr;
}

inline xcml::expr_ptr make_member_addr(xcml::expr_ptr const& expr, std::string_view name) {
    return make_addr_of(make_member_ref(expr, name));
}

inline xcml::pointer_ref_ptr member_array_ref(xcml::var_ref_ptr const& ref,
                                              std::string_view name, int index) {
    auto aref = new_member_array_ref();
    aref->value = ref;
    aref->member = name;

    auto pr = new_pointer_ref();

    if (index > 0) {
        pr->expr = plus_expr(aref, lit(index));
    } else {
        pr->expr = aref;
    }

    return pr;
}

inline array_ref_ptr make_array_ref(expr_ptr const& array, expr_ptr const& index) {
    auto ar = new_array_ref();
    ar->array = array;
    ar->index.push_back(index);
    return ar;
}

inline auto make_cast(std::string_view type, xcml::expr_ptr expr) {
    auto c = new_cast_expr();
    c->type = type;
    c->value = expr;
    return c;
}

inline auto make_cast(type_ptr type, xcml::expr_ptr expr) {
    return make_cast(type->type, expr);
}

inline pointer_ref_ptr make_deref(expr_ptr const& expr) {
    auto pr = new_pointer_ref();
    pr->expr = expr;
    return pr;
}

inline auto make_return(xcml::expr_ptr value) {
    auto stmt = new_return_stmt();
    stmt->value = value;
    return stmt;
}

template <class ArgIterator>
inline auto make_call(xcml::func_addr_ptr addr, ArgIterator arg_begin, ArgIterator arg_end) {
    auto call = new_function_call();
    call->function = addr;
    call->arguments.assign(arg_begin, arg_end);
    return call;
}

inline auto make_call(xcml::func_addr_ptr addr,
                      std::initializer_list<xcml::expr_ptr> args = {}) {
    return make_call(addr, args.begin(), args.end());
}

inline auto make_pragma(std::string_view value) {
    auto p = new_pragma();
    p->value = value;
    return p;
}

inline xcml::stmt_ptr as_stmt(xcml::node_ptr node) {
    if (auto stmt = xcml::stmt_node::dyncast(node)) {
        return stmt;
    }

    auto const errmsg = fmt::format("Not a statement node: {}", node->node_name());
    throw std::runtime_error(errmsg);
}

inline xcml::expr_ptr as_expr(xcml::node_ptr node) {
    if (auto expr = xcml::expr_node::dyncast(node)) {
        return expr;
    }

    auto const errmsg = fmt::format("Not an expression node: {}", node->node_name());
    throw std::runtime_error(errmsg);
}

inline xcml::var_ref_ptr add_local_var(xcml::compound_stmt_ptr const& scope,
                                       std::string const& type, std::string const& name,
                                       xcml::expr_ptr init = nullptr, bool push_front = false) {
    auto sym = new_symbol_id();
    sym->type = type;
    sym->name = name;

    auto decl = new_var_decl();
    decl->name = name;

    auto ref = new_var_ref();
    ref->name = name;

    if (push_front) {
        scope->symbols.push_front(sym);
        scope->declarations.push_front(decl);
    } else {
        scope->symbols.push_back(sym);
        scope->declarations.push_back(decl);
    }

    if (init) {
        push_expr(scope, assign_expr(ref, init));
    }

    return ref;
}

inline xcml::var_ref_ptr add_local_var(xcml::compound_stmt_ptr const& scope,
                                       type_ptr const& type, std::string const& name,
                                       xcml::expr_ptr init = nullptr) {
    return add_local_var(scope, type->type, name, init);
}

inline var_ref_ptr make_var_ref(std::string_view name) {
    auto vr = new_var_ref();
    vr->name = name;
    return vr;
}

inline expr_ptr make_var_addr(std::string_view name) {
    return make_addr_of(make_var_ref(name));
}

inline var_ref_ptr add_param(xcml::function_type_ptr const& node, std::string_view type,
                             std::string_view name) {
    auto param = new_param_node();
    param->name = name;
    param->type = type;

    node->params.push_back(param);

    return make_var_ref(param->name);
}

inline var_ref_ptr add_param(xcml::function_type_ptr const& node, type_ptr const& type,
                             std::string_view name) {
    return add_param(node, type->type, name);
}

inline var_ref_ptr add_param(xcml::function_type_ptr const& ft,
                             xcml::function_definition_ptr const& fd, std::string_view type,
                             std::string_view name) {
    auto const ref = add_param(ft, type, name);

    auto const sym = new_symbol_id();
    sym->name = name;
    sym->sclass = xcml::storage_class::param;
    sym->type = type;
    fd->symbols.push_back(sym);

    return ref;
}

inline var_ref_ptr add_param(xcml::function_type_ptr const& ft,
                             xcml::function_definition_ptr const& fd, type_ptr const& type,
                             std::string_view name) {
    return add_param(ft, fd, type->type, name);
}

template <class Node>
inline void add_param(Node const& node, std::string_view type, std::string_view name) {
    auto param = new_param_node();
    param->name = name;
    param->type = type;

    auto id = xcml::new_symbol_id();
    id->name = name;
    id->type = param->type;

    node->params.push_back(param);
    node->symbols.push_back(id);
}

template <class Node>
inline void add_param(Node const& node, type_ptr const& type, std::string_view name) {
    add_param(node, type->type, name);
}

inline func_addr_ptr make_func_addr(std::string_view name) {
    auto const fa = new_func_addr();
    fa->name = name;
    return fa;
}

inline std::optional<intmax_t> get_signed_value(xcml::int_constant_ptr const& c) {
    errno = 0;

    char* endptr = nullptr;
    auto const val = strtoimax(c->value.c_str(), &endptr, 0);

    if (errno || endptr != c->value.data() + c->value.size()) {
        return std::nullopt;
    }

    return val;
}

inline std::optional<uintmax_t> get_unsigned_value(xcml::int_constant_ptr const& c) {
    errno = 0;

    char* endptr = nullptr;
    auto const val = strtoumax(c->value.c_str(), &endptr, 0);

    if (errno || endptr != c->value.data() + c->value.size()) {
        return std::nullopt;
    }

    return val;
}

inline bool is_zero(xcml::int_constant_ptr const& c) {
    // get_signed_value() retruns nullopt
    // if c overflows from intmax_t (INTMAX_MAX < c <= UINTMAX_MAX).
    // In this case, c never be zero.
    return get_signed_value(c).value_or(1) == 0;
}

inline bool is_zero(xcml::expr_ptr const& expr) {
    if (auto c = xcml::int_constant::dyncast(expr)) {
        return is_zero(c);
    }
    return false;
}

}  // namespace xcml::utils
