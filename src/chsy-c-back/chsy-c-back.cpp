#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>
#include <vector>
#include <cxxopts.hpp>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <utils/target.hpp>
#include <xcml.hpp>
#include <xcml_recusive_visitor.hpp>

namespace {

struct options {
    utils::target target;
};

template <class... Args>
[[noreturn]] void fatal(char const* fmt, Args&&... args) {
    fmt::print(stderr, "Error: ");
    fmt::print(stderr, fmt::runtime(fmt), std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
    exit(1);
}

struct output_buffer {
    output_buffer() : type_(), body_(), type(type_), body(body_) {}

    output_buffer(output_buffer const&) = delete;

    output_buffer(output_buffer&&) = delete;

    output_buffer& operator=(output_buffer const&) = delete;

    output_buffer& operator=(output_buffer&&) = delete;

    friend std::ostream& operator<<(std::ostream& os, output_buffer const& out) {
        if (!out.pre_.empty()) {
            os.write(out.pre_.data(), out.pre_.size());
        }
        return os;
    }

    void flush() {
        pre_.insert(pre_.end(), type_.begin(), type_.end());
        type_.clear();
        pre_.push_back('\n');
        pre_.push_back('\n');

        pre_.insert(pre_.end(), body_.begin(), body_.end());
        body_.clear();
        pre_.push_back('\n');
    }

private:
    std::vector<char> pre_, type_, body_;

public:
    std::back_insert_iterator<std::vector<char>> type, body;
};

char const* to_s(xcml::storage_class sc) {
    switch (sc) {
        case xcml::storage_class::none:
            return "none";
        case xcml::storage_class::auto_:
            return "auto";
        case xcml::storage_class::param:
            return "param";
        case xcml::storage_class::extern_:
            return "extern_";
        case xcml::storage_class::extern_def:
            return "extern_def";
        case xcml::storage_class::static_:
            return "static_";
        case xcml::storage_class::register_:
            return "register_";
        case xcml::storage_class::label_:
            return "label_";
        case xcml::storage_class::tagname:
            return "tagname";
        case xcml::storage_class::moe:
            return "moe";
        case xcml::storage_class::typedef_name:
            return "typedef_name";
        default:
            return "invalid value";
    }
}

using type_map = std::unordered_map<std::string, std::shared_ptr<xcml::type_node>>;

inline xcml::type_ptr find_type(type_map const& map, std::string const& name) {
    auto it = map.find(name);
    if (it == map.end()) {
        fatal("type not found: {}", name);
    }
    return it->second;
}

struct collect_type_visitor : xcml::recursive_visitor<collect_type_visitor> {
    explicit collect_type_visitor(type_map const& map) : map_(map) {}

    std::vector<xcml::type_ptr> const& get() const {
        return types_;
    }

    xcml::node_ptr visit_xcml_program_node(xcml::xcml_program_node_ptr const& node,
                                           scope_ref scope) {
        for (auto const& sym : node->global_symbols) {
            add(sym->type);
        }
        return recursive_visitor::visit_xcml_program_node(node, scope);
    }

    xcml::node_ptr visit_compound_stmt(xcml::compound_stmt_ptr const& node, scope_ref scope) {
        for (auto const& sym : node->symbols) {
            add(sym->type);
        }
        return recursive_visitor::visit_compound_stmt(node, scope);
    }

    xcml::node_ptr visit_cast_expr(xcml::cast_expr_ptr const& node, scope_ref scope) {
        add(node->type);
        return recursive_visitor::visit_cast_expr(node, scope);
    }

private:
    void add(std::string const& type) {
        add(map_.at(type), type);
    }

    void add(xcml::type_ptr const& type, std::string const& name) {
        if (!added_.count(name)) {
            types_.push_back(type);
            added_.insert(name);
        }
    }

    type_map const& map_;
    std::unordered_set<std::string> added_;
    std::vector<xcml::type_ptr> types_;
};

struct symbol_scope {
    template <class Symbols>
    explicit symbol_scope(type_map const& type, Symbols const& syms) : type_(type) {
        init(syms);
    }

    template <class Symbols>
    explicit symbol_scope(symbol_scope const* parent, Symbols const& syms)
        : type_(parent->type_) {
        if (parent) {
            tab_ = parent->tab_;
            typetab_ = parent->typetab_;
        }
        init(syms);
    }

    struct lookup_result {
        xcml::symbol_id_ptr const sym;
        std::shared_ptr<xcml::type_node> const type;

        xcml::function_type_ptr function_type() const {
            if (auto ft = xcml::function_type::dyncast(type)) {
                return ft;
            }
            fatal("not a function type: {} referenced as symbol {}", type->type, sym->name);
        }
    };

    lookup_result lookup(std::string const& name) const {
        auto const sym = tab_.find(name);
        if (sym == tab_.end()) {
            fatal("symbol not found: {}", name);
        }

        auto const type = type_.find(sym->second->type);
        if (type == type_.end()) {
            fatal("type not found: {} referenced as symbol {}", sym->second->type, name);
        }

        return {sym->second, type->second};
    }

    lookup_result lookup_by_type(std::string const& name) const {
        auto const sym = typetab_.find(name);
        if (sym == typetab_.end()) {
            fatal("type not found: {}", name);
        }

        auto const type = type_.find(sym->second->type);
        if (type == type_.end()) {
            fatal("type not found: {} referenced as type {}", sym->second->type, name);
        }

        return {sym->second, type->second};
    }

private:
    template <class Symbols>
    void init(Symbols const& syms) {
        for (auto const& sym : syms) {
            tab_.insert_or_assign(sym->name, sym);

            if (sym->sclass == xcml::storage_class::tagname) {
                typetab_.insert_or_assign(sym->type, sym);
            }
        }
    }

    std::unordered_map<std::string, xcml::symbol_id_ptr> tab_;
    std::unordered_map<std::string, xcml::symbol_id_ptr> typetab_;
    type_map const& type_;
};

struct type_str {
    std::string left, right;

    std::string join(std::string_view var = {}) const {
        std::string res(left);

        if (!var.empty()) {
            res += ' ';
            res += var;
        }
        res += right;

        return res;
    }
};

struct type_decompiler : xcml::visitor<type_decompiler, type_str> {
    explicit type_decompiler(type_map const& types) : types_(types) {}

    type_str format_type(std::string const& name, symbol_scope const* scope) const {
        auto type = find_type(types_, name);
        auto bt = xcml::basic_type::dyncast(type);

        if (bt && bt->veclen > 0) {
            fatal("format_type: vector type is not supported: {}", type->type);
        }

        if (bt && bt->is_builtin) {
            type_str res;

            if (bt->is_const) {
                res.left += "const ";
            }
            res.left += bt->name;

            return res;
        } else if (bt) {
            auto res = format_type(bt->name, scope);

            if (bt->is_const) {
                res.left = "const " + res.left;
            }

            return res;
        } else if (auto t = xcml::struct_type::dyncast(type)) {
            auto const& res = scope->lookup_by_type(t->type);

            return {fmt::format(FMT_COMPILE("struct {}"), res.sym->name), {}};
        } else if (auto t = xcml::pointer_type::dyncast(type)) {
            auto ref = format_type(t->ref, scope);

            ref.left += '*';

            return ref;
        } else if (auto t = xcml::array_type::dyncast(type)) {
            auto elem = format_type(t->element_type, scope);

            if (t->array_size) {
                elem.right = fmt::format(FMT_COMPILE("[{}]"), t->array_size) + elem.right;
            } else {
                elem.right = "[]" + elem.right;
            }

            return elem;
        } else {
            fatal("format_type: not supported: {}", type->node_name());
        }
    }

private:
    type_map const& types_;
};

struct expr {
    explicit expr() = default;

    explicit expr(int p) : precedence(p) {}

    explicit expr(std::string_view s, int p) : str(s), precedence(p) {}

    std::string str;
    int precedence;
};

struct expr_decompiler : xcml::visitor<expr_decompiler, expr> {
    explicit expr_decompiler(type_map const& types) : types_(types) {}

#define OP(type, prec, opstr, is_pre)              \
    if (auto op = xcml::type::dyncast(node)) {     \
        auto const child = visit(op->expr, scope); \
                                                   \
        if constexpr (is_pre) {                    \
            res.str += opstr;                      \
        }                                          \
        if (child.precedence >= prec) {            \
            res.str += '(';                        \
        }                                          \
        res.str += child.str;                      \
        if (child.precedence >= prec) {            \
            res.str += ')';                        \
        }                                          \
        if constexpr (!is_pre) {                   \
            res.str += opstr;                      \
        }                                          \
                                                   \
        res.precedence = prec;                     \
    }
#define ELSE_OP(type, prec, opstr, is_pre) else OP(type, prec, opstr, is_pre)

    expr visit_unary_op(std::shared_ptr<xcml::unary_op> node, symbol_scope const* scope) {
        expr res;

        OP(log_not_expr, 3, "!", true)
        ELSE_OP(bit_not_expr, 3, "~", true)
        ELSE_OP(pre_incr_expr, 3, "++", true)
        ELSE_OP(post_incr_expr, 2, "++", false)
        ELSE_OP(pre_decr_expr, 3, "--", true)
        ELSE_OP(post_decr_expr, 2, "--", false)
        ELSE_OP(addr_of_expr, 3, "&", true)
        ELSE_OP(unary_minus_expr, 3, "-", true)
        ELSE_OP(unary_plus_expr, 3, "+", true)
        else {
            fatal("unknown unary operator: {}", node->node_name());
        }

        return res;
    }

#undef OP
#undef ELSE_OP

#define OP(type, prec, opstr, l_cond, r_cond)   \
    if (auto op = xcml::type::dyncast(node)) {  \
        auto const lhs = visit(op->lhs, scope); \
        auto const rhs = visit(op->rhs, scope); \
                                                \
        if (lhs.precedence l_cond prec) {       \
            res.str += '(';                     \
        }                                       \
        res.str += lhs.str;                     \
        if (lhs.precedence l_cond prec) {       \
            res.str += ')';                     \
        }                                       \
                                                \
        res.str += opstr;                       \
                                                \
        if (rhs.precedence r_cond prec) {       \
            res.str += '(';                     \
        }                                       \
        res.str += rhs.str;                     \
        if (rhs.precedence r_cond prec) {       \
            res.str += ')';                     \
        }                                       \
                                                \
        res.precedence = prec;                  \
    }
#define LEFT(type, prec, opstr) OP(type, prec, opstr, >, >=)
#define ELSE_LEFT(type, prec, opstr) else OP(type, prec, opstr, >, >=)
#define RIGHT(type, prec, opstr) OP(type, prec, opstr, >=, >)
#define ELSE_RIGHT(type, prec, opstr) else OP(type, prec, opstr, >=, >)

    expr visit_binary_op(std::shared_ptr<xcml::binary_op> node, symbol_scope const* scope) {
        expr res;

        RIGHT(asg_bitand_expr, 16, " &= ")
        ELSE_RIGHT(asg_bitor_expr, 16, " |= ")
        ELSE_RIGHT(asg_bitxor_expr, 16, " ^= ")
        ELSE_RIGHT(asg_div_expr, 16, " /= ")
        ELSE_RIGHT(asg_lshift_expr, 16, " <<= ")
        ELSE_RIGHT(asg_minus_expr, 16, " -= ")
        ELSE_RIGHT(asg_mul_expr, 16, " *= ")
        ELSE_RIGHT(asg_plus_expr, 16, " += ")
        ELSE_RIGHT(asg_rshift_expr, 16, " >>= ")
        ELSE_RIGHT(assign_expr, 16, " = ")
        ELSE_LEFT(bit_and_expr, 11, " & ")
        ELSE_LEFT(bit_or_expr, 13, " | ")
        ELSE_LEFT(bit_xor_expr, 12, " ^ ")
        ELSE_LEFT(div_expr, 5, " / ")
        ELSE_LEFT(log_and_expr, 14, " && ")
        ELSE_LEFT(log_eq_expr, 10, " == ")
        ELSE_LEFT(log_neq_expr, 10, " != ")
        ELSE_LEFT(log_ge_expr, 9, " >= ")
        ELSE_LEFT(log_gt_expr, 9, " > ")
        ELSE_LEFT(log_le_expr, 9, " <= ")
        ELSE_LEFT(log_lt_expr, 9, " < ")
        ELSE_LEFT(log_or_expr, 15, " || ")
        ELSE_LEFT(lshift_expr, 7, " << ")
        ELSE_LEFT(mod_expr, 5, " % ")
        ELSE_LEFT(minus_expr, 6, " - ")
        ELSE_LEFT(mul_expr, 5, " * ")
        ELSE_LEFT(plus_expr, 6, " + ")
        ELSE_LEFT(rshift_expr, 7, " >> ")
        else {
            fatal("unknown binary operator: {}", node->node_name());
        }

        return res;
    }

    expr visit_var_ref(xcml::var_ref_ptr node, symbol_scope const*) {
        return expr(node->name, 0);
    }

    expr visit_var_addr(xcml::var_addr_ptr node, symbol_scope const*) {
        return expr('&' + node->name, 3);
    }

    expr visit_func_addr(xcml::func_addr_ptr node, symbol_scope const*) {
        return expr(node->name, 0);
    }

    expr visit_array_addr(xcml::array_addr_ptr node, symbol_scope const*) {
        return expr(node->name, 0);
    }

    expr visit_member_array_ref(xcml::member_array_ref_ptr node, symbol_scope const* scope) {
        return member_ref(node, scope);
    }

    expr visit_member_addr(xcml::member_addr_ptr const& node, symbol_scope const* scope) {
        expr res(3);
        auto const val = visit(node->value, scope);

        res.str = '&';
        if (val.precedence >= res.precedence) {
            res.str += '(';
        }
        res.str += val.str;
        if (val.precedence >= res.precedence) {
            res.str += ')';
        }

        return res;
    }

    bool is_hex(std::string_view s) {
        return s.size() >= 2 && s.substr(0, 2) == "0x";
    }

    expr visit_int_constant(xcml::int_constant_ptr node, symbol_scope const*) {
        if (node->type == "unsigned") {
            auto const radix = is_hex(node->value) ? 16 : 10;
            return expr(std::to_string(std::stoul(node->value, nullptr, radix)) + 'U', 0);
        }

        if (node->type == "long") {
            auto const radix = is_hex(node->value) ? 16 : 10;
            return expr(std::to_string(std::stol(node->value, nullptr, radix)) + 'L', 0);
        }

        if (node->type == "unsigned_long") {
            auto const radix = is_hex(node->value) ? 16 : 10;
            return expr(std::to_string(std::stoul(node->value, nullptr, radix)) + "UL", 0);
        }

        auto const radix = is_hex(node->value) ? 16 : 10;
        return expr(std::to_string(std::stoi(node->value, nullptr, radix)), 0);
    }

    expr visit_long_long_constant(xcml::long_long_constant_ptr node, symbol_scope const*) {
        errno = 0;
        unsigned long long val = 0;
        char* end;

        val |= strtoull(node->value.c_str(), &end, 16) << 32;
        if (errno) {
            fatal("could not convert to an integer: {}", node->value);
        }

        val |= strtoull(end, nullptr, 16);
        if (errno) {
            fatal("could not convert to an integer: {}", node->value);
        }

        return expr(std::to_string(val) + "ULL", 0);
    }

    expr visit_float_constant(xcml::float_constant_ptr node, symbol_scope const*) {
        return expr(node->value, 0);
    }

    expr visit_string_constant(xcml::string_constant_ptr node, symbol_scope const*) {
        expr res(0);

        res.str += '"';
        for (auto const& ch : node->value) {
            if (ch == '\n') {
                res.str += "\\n";
            } else if (ch == '\r') {
                res.str += "\\r";
            } else if (ch == '\t') {
                res.str += "\\t";
            } else if (ch == '\v') {
                res.str += "\\v";
            } else if (ch == '\\') {
                res.str += "\\\\";
            } else {
                res.str += ch;
            }
        }
        res.str += '"';

        return res;
    }

    template <class NodePtr>
    expr member_ref(NodePtr const& node, symbol_scope const* scope) {
        auto const va = xcml::var_addr::dyncast(node->value);
        auto const ao = xcml::addr_of_expr::dyncast(node->value);

        auto const val = ao ? visit(ao->expr, scope) : visit(node->value, scope);

        expr res(2);

        if (va) {
            res.str += va->name;
        } else {
            if (val.precedence > res.precedence) {
                res.str += '(';
            }
            res.str += val.str;
            if (val.precedence > res.precedence) {
                res.str += ')';
            }
        }

        if (va || ao) {
            res.str += '.';
        } else {
            res.str += "->";
        }

        res.str += node->member;

        return res;
    }

    expr visit_member_ref(xcml::member_ref_ptr node, symbol_scope const* scope) {
        return member_ref(node, scope);
    }

    expr visit_pointer_ref(xcml::pointer_ref_ptr node, symbol_scope const* scope) {
        auto const val = visit(node->expr, scope);

        expr res(3);

        res.str += '*';
        if (val.precedence > res.precedence) {
            res.str += '(';
        }
        res.str += val.str;
        if (val.precedence > res.precedence) {
            res.str += ')';
        }

        return res;
    }

    expr visit_function_call(xcml::function_call_ptr node, symbol_scope const* scope) {
        auto const fn = visit(node->function, scope);

        expr res(2);

        if (fn.precedence > res.precedence) {
            res.str += '(';
        }
        res.str += fn.str;
        if (fn.precedence > res.precedence) {
            res.str += ')';
        }

        res.str += '(';
        for (size_t i = 0; i < node->arguments.size(); i++) {
            auto const& arg = node->arguments.at(i);

            if (i > 0) {
                res.str += ',';
            }
            res.str += visit(arg, scope).str;
        }
        res.str += ')';

        return res;
    }

    expr visit_cast_expr(xcml::cast_expr_ptr node, symbol_scope const* scope) {
        auto const val = visit(node->value, scope);

        expr res(3);

        res.str += '(';
        type_decompiler dec(types_);
        res.str += dec.format_type(node->type, scope).join();
        res.str += ')';

        if (val.precedence > res.precedence) {
            res.str += '(';
        }
        res.str += val.str;
        if (val.precedence > res.precedence) {
            res.str += ')';
        }

        return res;
    }

    expr visit_array_ref(xcml::array_ref_ptr node, symbol_scope const* scope) {
        auto const arr = visit(node->array, scope);

        expr res(2);

        if (arr.precedence > res.precedence) {
            res.str += '(';
        }
        res.str += arr.str;
        if (arr.precedence > res.precedence) {
            res.str += ')';
        }

        for (auto const& i : node->index) {
            res.str += '[';
            res.str += visit(i, scope).str;
            res.str += ']';
        }

        return res;
    }

    expr visit_cond_expr(xcml::cond_expr_ptr const& node, symbol_scope const* scope) {
        auto const cond = visit(node->cond, scope);
        auto const true_ = visit(node->true_, scope);
        auto const false_ = visit(node->false_, scope);

        expr res(16);

        if (cond.precedence > res.precedence) {
            res.str += '(';
        }
        res.str += cond.str;
        if (cond.precedence > res.precedence) {
            res.str += ')';
        }

        res.str += " ? ";

        if (true_.precedence > res.precedence) {
            res.str += '(';
        }
        res.str += true_.str;
        if (true_.precedence > res.precedence) {
            res.str += ')';
        }

        res.str += " : ";

        if (false_.precedence > res.precedence) {
            res.str += '(';
        }
        res.str += false_.str;
        if (false_.precedence > res.precedence) {
            res.str += ')';
        }

        return res;
    }

#undef OP
#undef LEFT
#undef RIGHT

    std::string str;

private:
    type_map const& types_;
};

#define FORMAT(fmt_str, ...) fmt::format_to(out_.body, FMT_COMPILE(fmt_str), __VA_ARGS__)
#define PRINT(str) fmt::format_to(out_.body, FMT_COMPILE("{}"), (str))

#define T_FORMAT(fmt_str, ...) fmt::format_to(out_.type, FMT_COMPILE(fmt_str), __VA_ARGS__)
#define T_PRINT(str) fmt::format_to(out_.type, FMT_COMPILE("{}"), (str))

struct decompiler : xcml::visitor<decompiler, void> {
    explicit decompiler(options const& opt) : opt_(opt) {}

    void operator()(xcml::xcml_program_node_ptr node) {
        init();
        visit(node, nullptr);
    }

    void visit_xcml_program_node(xcml::xcml_program_node_ptr node,
                                 [[maybe_unused]] symbol_scope const* _scope) {
        assert(_scope == nullptr);

        for (auto const& decl : node->preamble) {
            visit(decl, nullptr);
        }

        out_.flush();

        for (auto const& sym : node->type_table) {
            visit(sym, nullptr);
        }

        symbol_scope scope(types_, node->global_symbols);

        collect_type_visitor vis(types_);
        vis.apply(node);
        for (auto const& type : vis.get()) {
            print_type_definition(type, &scope);
        }

        for (auto const& decl : node->global_declarations) {
            visit(decl, &scope);
        }

        out_.flush();
    }

    void add_type(std::shared_ptr<xcml::type_node> const& node) {
        auto const [_, inserted] = types_.insert_or_assign(node->type, node);
        if (!inserted) {
            fmt::print(stderr, "Error: duplicate type: {}\n", node->type);
            exit(1);
        }
    }

    void visit_function_type(xcml::function_type_ptr const& node, symbol_scope const*) {
        add_type(node);
    }

    void visit_pointer_type(xcml::pointer_type_ptr const& node, symbol_scope const*) {
        add_type(node);
    }

    void visit_struct_type(xcml::struct_type_ptr const& node, symbol_scope const*) {
        add_type(node);
    }

    void visit_basic_type(xcml::basic_type_ptr const& node, symbol_scope const*) {
        add_type(node);
    }

    void visit_array_type(xcml::array_type_ptr const& node, symbol_scope const*) {
        add_type(node);
    }

    void print_func(symbol_scope::lookup_result const& info, symbol_scope const* scope,
                    std::vector<xcml::gcc_attribute_ptr> const& attrs,
                    std::vector<xcml::cuda_attribute_ptr> const& cuda_attrs) {
        auto const ft = info.function_type();

        switch (info.sym->sclass) {
            case xcml::storage_class::extern_def:
                break;
            case xcml::storage_class::static_:
                PRINT("static ");
                break;
            default:
                fatal("{}: not supported: {}", __func__, to_s(info.sym->sclass));
                break;
        }

        if (is_cuda(opt_.target) || is_hip(opt_.target)) {
            for (auto const& attr : cuda_attrs) {
                FORMAT("{} ", attr->value);
            }
        } else {
            assert(cuda_attrs.empty());
        }

        if (!attrs.empty()) {
            PRINT("__attribute__((");
            for (size_t i = 0; i < attrs.size(); i++) {
                if (i > 0) {
                    PRINT(",");
                }
                PRINT(attrs.at(i)->name);
            }
            PRINT(")) ");
        }

        print_type(ft->return_type, "", scope);
        PRINT(" ");

        FORMAT("{}(", info.sym->name);
        for (size_t i = 0; i < ft->params.size(); i++) {
            if (i > 0) {
                PRINT(",");
            }

            auto const& p = ft->params.at(i);

            if (auto param = xcml::param_node::dyncast(p)) {
                print_type(param->type, param->name, scope);
            } else if (xcml::ellipsis::dyncast(p)) {
                PRINT("...");
            }
        }
        PRINT(")");
    }

    void visit_function_definition(xcml::function_definition_ptr node,
                                   symbol_scope const* scope) {
        auto const& info = scope->lookup(node->name);
        auto const ft = info.function_type();

        print_loc(node);
        print_func(info, scope, node->attributes, ft->cuda_attrs);

        symbol_scope new_scope(scope, node->symbols);

        visit(node->body, &new_scope);

        sep();
    }

    void visit_function_decl(xcml::function_decl_ptr node, symbol_scope const* scope) {
        auto const info = scope->lookup(node->name);
        auto const ft = info.function_type();

        if (is_cxx(opt_.target) && node->extern_c) {
            PRINT("extern \"C\" ");
        }

        print_func(info, scope, {}, ft->cuda_attrs);
        PRINT(";");
        sep();
    }

    void visit_cpp_include(xcml::cpp_include_ptr const& node, symbol_scope const*) {
        FORMAT("\n#include <{}>\n", node->name);
    }

    void visit_compound_stmt(xcml::compound_stmt_ptr node, symbol_scope const* scope) {
        PRINT("{");

        symbol_scope new_scope(scope, node->symbols);

        for (auto const& decl : node->declarations) {
            visit(decl, &new_scope);
        }

        if (!node->declarations.empty() && !node->body.empty()) {
            sep();
        }

        for (auto const& stmt : node->body) {
            visit(stmt, &new_scope);
        }

        PRINT("}");
    }

    void visit_var_decl(xcml::var_decl_ptr node, symbol_scope const* scope) {
        auto const& info = scope->lookup(node->name);

        if (info.sym->sclass == xcml::storage_class::extern_) {
            PRINT("extern ");
        } else if (info.sym->sclass == xcml::storage_class::static_) {
            PRINT("static ");
        }

        if (auto const& attrs = info.sym->gccAttributes; !attrs.empty()) {
            PRINT("__attribute__((");
            for (size_t i = 0; i < attrs.size(); i++) {
                if (i > 0) {
                    PRINT(",");
                }
                PRINT(attrs.at(i)->name);
            }
            PRINT(")) ");
        }

        auto const& cuda_attrs = info.sym->cudaAttributes;

        for (size_t i = 0; i < cuda_attrs.size(); i++) {
            PRINT(cuda_attrs.at(i)->value);
            PRINT(" ");
        }

        print_type(info.type->type, info.sym->name, scope);

        PRINT(";");
    }

    void visit_expr_stmt(xcml::expr_stmt_ptr node, symbol_scope const* scope) {
        expr_decompiler dec(types_);
        auto const expr = dec.visit(node->expr, scope);
        PRINT(expr.str);
        PRINT(";");

        print_loc(node->expr);
    }

    void visit_return_stmt(xcml::return_stmt_ptr node, symbol_scope const* scope) {
        PRINT("return");

        if (node->value) {
            PRINT(" ");
            expr_decompiler dec(types_);
            PRINT(dec.visit(node->value, scope).str);
        }

        PRINT(";");
    }

    void visit_for_stmt(xcml::for_stmt_ptr node, symbol_scope const* scope) {
        if (node->pragma.size()) {
            PRINT("\n");
            for (auto const& pragma : node->pragma) {
                FORMAT("#pragma {}\n", pragma->value);
            }
        }

        PRINT("for(");

        if (node->init) {
            expr_decompiler dec(types_);
            PRINT(dec.visit(node->init, scope).str);
        }

        PRINT(";");

        if (node->condition) {
            expr_decompiler dec(types_);
            PRINT(dec.visit(node->condition, scope).str);
        }

        PRINT(";");

        if (node->iter) {
            expr_decompiler dec(types_);
            PRINT(dec.visit(node->iter, scope).str);
        }

        PRINT(")");

        visit(node->body, scope);
    }

    void visit_while_stmt(xcml::while_stmt_ptr node, symbol_scope const* scope) {
        PRINT("while(");

        if (node->condition) {
            expr_decompiler dec(types_);
            PRINT(dec.visit(node->condition, scope).str);
        }

        PRINT(")");

        visit(node->body, scope);
    }

    void visit_if_stmt(xcml::if_stmt_ptr node, symbol_scope const* scope) {
        PRINT("if(");

        if (node->condition) {
            expr_decompiler dec(types_);
            PRINT(dec.visit(node->condition, scope).str);
        }

        PRINT(")");

        if (node->then) {
            visit(node->then, scope);
        }

        if (node->else_ && !is_empty(node->else_)) {
            PRINT("else");

            visit(node->else_, scope);
        }
    }

    void visit_switch_stmt(xcml::switch_stmt_ptr const& node, symbol_scope const* scope) {
        PRINT("switch(");

        expr_decompiler dec(types_);
        PRINT(dec.visit(node->value, scope).str);

        PRINT(")");

        visit(node->body, scope);
    }

    void visit_case_label_stmt(xcml::case_label_stmt_ptr const& node,
                               symbol_scope const* scope) {
        PRINT("case ");

        expr_decompiler dec(types_);
        PRINT(dec.visit(node->value, scope).str);

        PRINT(":");
    }

    void visit_default_stmt(xcml::default_stmt_ptr const&, symbol_scope const*) {
        PRINT("default:");
    }

    void visit_break_stmt(xcml::break_stmt_ptr const&, symbol_scope const*) {
        PRINT("break;");
    }

    void visit_continue_stmt(xcml::continue_stmt_ptr const&, symbol_scope const*) {
        PRINT("continue;");
    }

    void visit_code(xcml::code_ptr node, symbol_scope const*) {
        PRINT(node->value);
    }

    output_buffer const& output() const {
        return out_;
    }

private:
    void print_loc(xcml::node_ptr node) {
        if (!node->file.empty() && node->line >= 0) {
            FORMAT("/* {}:{} */\n", node->file, node->line);
        }
    }

    bool is_empty(xcml::compound_stmt_ptr const& node) const {
        return node->symbols.empty() && node->body.empty() && node->declarations.empty();
    }

    void init() {
#define ADD(x, y)                            \
    {                                        \
        auto t = xcml::new_basic_type();     \
        t->type = (x);                       \
        t->name = (y);                       \
        t->is_builtin = true;                \
        types_.insert_or_assign(t->type, t); \
    }

        ADD("void", "void");
        if (is_cxx(opt_.target) || is_cuda(opt_.target)) {
            ADD("_Bool", "bool");
        } else {
            ADD("_Bool", "_Bool");
        }
        ADD("char", "char");
        ADD("unsigned_char", "unsigned char");
        ADD("short", "short");
        ADD("unsigned_short", "unsigned short");
        ADD("int", "int");
        ADD("unsigned", "unsigned int");
        ADD("long", "long");
        ADD("unsigned_long", "unsigned long");
        ADD("long_long", "long long");
        ADD("unsigned_long_long", "unsigned long long");
        ADD("float", "float");
        ADD("double", "double");
        ADD("long_double", "long double");
    }

    void sep() {
        PRINT("\n\n");
    }

    type_str format_type(std::string const& name, symbol_scope const* scope) const {
        type_decompiler dec(types_);
        return dec.format_type(name, scope);
    }

    void print_type(std::string const& type, std::string_view var,
                    symbol_scope const* scope) const {
        PRINT(format_type(type, scope).join(var));
    }

    void print_type_definition(xcml::type_ptr node, symbol_scope const* scope) {
        if (shown_types_.find(node->type) == shown_types_.end()) {
            if (auto t = xcml::struct_type::dyncast(node)) {
                print_type_definition(t, scope);
            } else if (auto t = xcml::pointer_type::dyncast(node)) {
                print_type_definition(t, scope);
            } else if (auto t = xcml::basic_type::dyncast(node)) {
                if (!t->is_builtin) {
                    print_type_definition(find_type(types_, t->name), scope);
                }
            }
            shown_types_.insert(node->type);
        }
    }

    void print_type_definition(xcml::pointer_type_ptr node, symbol_scope const* scope) {
        print_type_definition(types_.at(node->ref), scope);
    }

    void print_type_definition(xcml::struct_type_ptr node, symbol_scope const* scope) {
        auto info = scope->lookup_by_type(node->type);

        for (auto const& sym : node->symbols) {
            print_type_definition(types_.at(sym->type), scope);
        }

        T_FORMAT("struct {}", info.sym->name);

        if (node->symbols.empty()) {
            T_PRINT(";\n\n");
        } else {
            T_PRINT("{");
            for (auto const& sym : node->symbols) {
                T_PRINT(format_type(sym->type, scope).join(sym->name));
                T_PRINT(";");
            }
            T_PRINT("};\n\n");
        }
    }

    output_buffer out_;
    type_map types_;
    std::unordered_set<std::string> shown_types_;
    options const& opt_;
};

}  // namespace

#ifdef IMPLEMENT_MAIN
int main(int argc, char** argv)
#else
int cback_main(int argc, char** argv)
#endif
{
    cxxopts::Options options(argv[0]);
    options.add_options()("input", "input file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("o,output", "output file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("t,target", "target", cxxopts::value<std::string>());
    options.parse_positional("input");
    auto opts = options.parse(argc, argv);

    auto const& input = opts["input"].as<std::string>();
    auto const& output = opts["output"].as<std::string>();
    auto const& target_str = opts["target"].as<std::string>();

    auto const target = utils::from_string(target_str);
    if (target == utils::target::NONE) {
        fmt::print(stderr, "{}: Unknown target: {}\n", argv[0], target_str);
        return 1;
    }

    auto doc = xcml::read_xml(input);
    auto prg = xcml::xml_to_prg(doc);

    ::options opt;
    opt.target = target;

    decompiler dec(opt);
    dec(prg);

    if (output == "-") {
        std::cout << dec.output();
    } else {
        std::ofstream ofs;
        ofs.exceptions(ofs.failbit | ofs.badbit);
        ofs.open(output);

        ofs << dec.output();
    }

    return 0;
}
