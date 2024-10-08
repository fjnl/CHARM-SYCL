#include <xcml_utils.hpp>
#include "xcml_recusive_visitor.hpp"

namespace {

struct compute_type_visitor : xcml::visitor<compute_type_visitor, xcml::type_ptr> {
    explicit compute_type_visitor(xcml::symbol_scope const& scope) : scope_(scope) {}

    xcml::type_ptr visit_member_ref(xcml::member_ref_ptr const& node) {
        auto ty = visit(node->value);

        for (;;) {
            if (auto pt = xcml::pointer_type::dyncast(ty)) {
                ty = scope_.base_of(pt);
                continue;
            }

            break;
        }

        auto st = xcml::struct_type::dyncast(ty);

        for (auto const& sym : st->symbols) {
            if (sym->name == node->member) {
                return scope_.type_of(sym);
            }
        }
        return nullptr;
    }

    xcml::type_ptr visit_var_ref(xcml::var_ref_ptr const& node) {
        auto res = scope_.lookup(node->name);
        return res.type;
    }

private:
    xcml::symbol_scope const& scope_;
};

}  // namespace

namespace xcml {

void recursive_visitor_base::init(xcml_program_node_ptr const& node) {
    root_ = node;
    set_nextid(node->gensym_id);

    static std::pair<char const*, char const*> const BASIC_TYPES[]{
        {"void", "void"},
        {"_Bool", "int"},
        {"char", "char"},
        {"unsigned_char", "unsigned char"},
        {"short", "short"},
        {"unsigned_short", "unsigned short"},
        {"int", "int"},
        {"unsigned", "unsigned int"},
        {"long", "long"},
        {"unsigned_long", "unsigned long"},
        {"long_long", "long long"},
        {"unsigned_long_long", "unsigned long long"},
        {"float", "float"},
        {"double", "double"},
        {"long_double", "long double"},
    };

    for (auto const& ty : BASIC_TYPES) {
        auto t = new_basic_type();
        t->type = ty.first;
        t->name = ty.second;
        t->is_builtin = true;
        type_map_.insert_or_assign(t->type, t);
    }
}

void recursive_visitor_base::fini(xcml_program_node_ptr const& node) {
    utils::sort_decls(node);
    node->gensym_id = get_nextid();
}

xcml::xcml_program_node_ptr const& recursive_visitor_base::root() {
    return root_;
}

symbol_scope::lookup_result recursive_visitor_base::lookup(scope_ref scope,
                                                           std::string const& name) {
    return scope.value().get().lookup(name);
}

basic_type_ptr recursive_visitor_base::create_basic_type(std::string const& name, bool is_const,
                                                         bool is_builtin) {
    auto bt = new_basic_type();
    bt->type = xbasic_type();
    bt->name = name;
    bt->is_const = is_const;
    bt->is_builtin = is_builtin;

    add_type(bt);
    root_->type_table.push_back(bt);

    return bt;
}

function_type_ptr recursive_visitor_base::create_function_type() {
    auto const ft = new_function_type();
    ft->type = xfunc_type();

    add_type(ft);
    root_->type_table.push_back(ft);

    return ft;
}

pointer_type_ptr recursive_visitor_base::create_pointer_type(xcml::type_ptr const& ref) {
    return create_pointer_type(ref->type);
}

pointer_type_ptr recursive_visitor_base::create_pointer_type(std::string const& ref) {
    auto pt = new_pointer_type();
    pt->type = xptr_type();
    pt->ref = ref;

    add_type(pt);
    root_->type_table.push_back(pt);

    return pt;
}

type_ptr recursive_visitor_base::find_pointer_type(type_ptr const& type) const {
    for (auto const& t : root_->type_table) {
        if (auto pt = pointer_type::dyncast(t)) {
            if (pt->ref == type->type) {
                return pt;
            }
        }
    }

    return nullptr;
}

type_ptr recursive_visitor_base::get_pointer_type(type_ptr const& type) {
    if (auto pt = find_pointer_type(type)) {
        return pt;
    }

    return create_pointer_type(type->type);
}

type_ptr const& recursive_visitor_base::get_basic_type(std::string const& name) const {
    return type_map_.at(name);
}

void recursive_visitor_base::add_type(std::shared_ptr<type_node> const& node) {
    auto const [_, inserted] = type_map_.insert_or_assign(node->type, node);
    if (!inserted) {
        fmt::print(stderr, "Error: duplicate type: {}\n", node->type);
        exit(1);
    }
}

xcml::type_ptr recursive_visitor_base::compute_type2(expr_ptr const& expr,
                                                     symbol_scope const& scope) {
    return compute_type_visitor(scope).visit(expr);
}

std::string recursive_visitor_base::compute_type(expr_ptr const& expr,
                                                 symbol_scope const& scope) {
    return compute_type2(expr, scope)->type;
}

type_ptr const& recursive_visitor_base::get_type(std::string const& name) const {
    return type_map_.at(name);
}

}  // namespace xcml
