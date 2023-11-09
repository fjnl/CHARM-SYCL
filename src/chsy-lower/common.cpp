#include <unordered_set>
#include <xcml.hpp>
#include <xcml_recusive_visitor.hpp>
#include "chsy-lower.hpp"

namespace {

struct collect_used_functions_visitor final
    : xcml::recursive_visitor<collect_used_functions_visitor> {
    explicit collect_used_functions_visitor(std::unordered_set<std::string>& set) : set_(set) {}

    xcml::node_ptr visit_function_definition(xcml::function_definition_ptr const& node,
                                             scope_ref scope) {
        bool inserted = false;

        if (!inserted) {
            if (lookup(scope, node->name).sym->sclass == xcml::storage_class::extern_def) {
                set_.insert(node->name);
                inserted = true;
            }
        }

        if (!inserted) {
            for (auto const& attr : node->attributes) {
                if (attr->name == "constructor" || attr->name == "destructor") {
                    set_.insert(node->name);
                    inserted = true;
                    break;
                }
            }
        }

        return recursive_visitor::visit_function_definition(node, scope);
    }

    xcml::node_ptr visit_func_addr(xcml::func_addr_ptr const& node, scope_ref) {
        set_.insert(node->name);
        return node;
    }

private:
    std::unordered_set<std::string>& set_;
};

struct remove_unused_functions_visitor final
    : xcml::recursive_visitor<remove_unused_functions_visitor> {
    explicit remove_unused_functions_visitor(std::unordered_set<std::string> const& set)
        : set_(set) {}

    xcml::node_ptr visit_xcml_program_node(xcml::xcml_program_node_ptr const& node, scope_ref) {
        for (auto it = node->global_declarations.begin();
             it != node->global_declarations.end();) {
            auto& decl = *it;

            auto const fdecl = xcml::function_decl::dyncast(decl);
            auto const fdefi = xcml::function_definition::dyncast(decl);
            auto const remove =
                (fdecl && !set_.count(fdecl->name)) || (fdefi && !set_.count(fdefi->name));

            if (remove) {
                it = node->global_declarations.erase(it);
            } else {
                ++it;
            }
        }

        return node;
    }

private:
    std::unordered_set<std::string> const& set_;
};

struct fix_integral_casts_visitor final : xcml::recursive_visitor<fix_integral_casts_visitor> {
    xcml::node_ptr visit_cast_expr(xcml::cast_expr_ptr const& node, scope_ref scope) {
        using namespace xcml::utils;

        if (is_integral_type(node->type)) {
            if (auto ic = xcml::int_constant::dyncast(node->value)) {
                auto const res = new_int_constant();
                res->type = node->type;

                if (is_singed_int_type(ic->type)) {
                    auto const val = get_signed_value(ic).value();

                    if (res->type == "char") {
                        res->value = std::to_string(static_cast<char>(val));
                    } else if (res->type == "short") {
                        res->value = std::to_string(static_cast<short>(val));
                    } else if (res->type == "int") {
                        res->value = std::to_string(static_cast<int>(val));
                    } else if (res->type == "long") {
                        res->value = std::to_string(static_cast<long>(val));
                    } else {
                        res->value = std::to_string(static_cast<long long>(val));
                    }
                } else {
                    auto const val = get_unsigned_value(ic).value();

                    if (res->type == "_Bool") {
                        res->value = std::to_string(static_cast<bool>(val));
                    } else if (res->type == "unsigned_char") {
                        res->value = std::to_string(static_cast<unsigned char>(val));
                    } else if (res->type == "unsigned_short") {
                        res->value = std::to_string(static_cast<unsigned short>(val));
                    } else if (res->type == "unsigned") {
                        res->value = std::to_string(static_cast<unsigned int>(val));
                    } else if (res->type == "unsigned_long") {
                        res->value = std::to_string(static_cast<unsigned long>(val));
                    } else {
                        res->value = std::to_string(static_cast<unsigned long long>(val));
                    }
                }

                return res;
            }
        }

        return recursive_visitor::visit_cast_expr(node, scope);
    }
};

struct fix_address_of_visitor final : xcml::recursive_visitor<fix_address_of_visitor> {
    xcml::node_ptr visit_pointer_ref(xcml::pointer_ref_ptr const& node, scope_ref scope) {
        expr_(node->expr, scope);

        if (auto va = xcml::var_addr::dyncast(node->expr)) {
            auto vr = xcml::new_var_ref();
            vr->scope = va->scope;
            vr->name = va->name;
            return vr;
        }

        if (auto ma = xcml::member_addr::dyncast(node->expr)) {
            auto mr = xcml::new_member_ref();
            mr->member = ma->member;
            mr->value = ma->value;
            return mr;
        }

        if (auto ao = xcml::addr_of_expr::dyncast(node->expr)) {
            return ao->expr;
        }

        return node;
    }
};

struct optimize_compounds_visitor final : xcml::recursive_visitor<optimize_compounds_visitor> {
    xcml::node_ptr visit_compound_stmt(xcml::compound_stmt_ptr const& node, scope_ref scope) {
        auto res = recursive_visitor::visit_compound_stmt(node, scope);

        if (auto compound = xcml::compound_stmt::dyncast(res)) {
            std::list<xcml::stmt_ptr> new_body;

            for (auto const& stmt : compound->body) {
                auto const child = xcml::compound_stmt::dyncast(stmt);
                auto const collapse = child && stmt == compound->body.front();
                auto const expand =
                    child && child->declarations.empty() && child->symbols.empty();

                if (collapse) {
                    append(compound->declarations, child->declarations);
                    append(compound->symbols, child->symbols);
                }

                if (collapse || expand) {
                    append(new_body, child->body);
                } else {
                    new_body.push_back(stmt);
                }
            }

            compound->body = std::move(new_body);
            return compound;
        }

        return res;
    }

private:
    template <class T, class U>
    static void append(std::vector<T>& vec1, std::vector<U> const& vec2) {
        vec1.insert(vec1.end(), vec2.cbegin(), vec2.cend());
    }

    template <class T, class U>
    static void append(std::list<T>& vec1, std::list<U> const& vec2) {
        vec1.insert(vec1.end(), vec2.cbegin(), vec2.cend());
    }
};

struct optimize_casts_visitor final : xcml::recursive_visitor<optimize_casts_visitor> {
    xcml::node_ptr visit_cast_expr(xcml::cast_expr_ptr const& node, scope_ref scope) {
        if (xcml::pointer_type::dyncast(get_type(node->type))) {
            if (auto nested = xcml::cast_expr::dyncast(node->value);
                nested && xcml::pointer_type::dyncast(get_type(nested->type))) {
                node->value = nested->value;
                return visit(node, scope);
            }
        }

        return recursive_visitor::visit_cast_expr(node, scope);
    }
};

struct optimize_base_casts_visitor final
    : xcml::recursive_visitor<optimize_base_casts_visitor> {
    xcml::node_ptr visit_member_ref(xcml::member_ref_ptr const& node, scope_ref scope) {
        if (auto val = xcml::cast_expr::dyncast(node->value); val && val->to_base) {
            node->value = val->value;
            return visit(node, scope);
        }

        return recursive_visitor::visit_member_ref(node, scope);
    }
};

}  // namespace

xcml::xcml_program_node_ptr remove_unused_functions(xcml::xcml_program_node_ptr const& prg) {
    std::unordered_set<std::string> set;
    collect_used_functions_visitor v1(set);
    remove_unused_functions_visitor v2(set);
    return apply_visitors(prg, v1, v2);
}

xcml::xcml_program_node_ptr fix_integral_casts(xcml::xcml_program_node_ptr const& prg) {
    fix_integral_casts_visitor v;
    return apply_visitors(prg, v);
}

xcml::xcml_program_node_ptr fix_address_of(xcml::xcml_program_node_ptr const& prg) {
    fix_address_of_visitor v;
    return apply_visitors(prg, v);
}

xcml::xcml_program_node_ptr optimize_compounds(xcml::xcml_program_node_ptr const& prg) {
    optimize_compounds_visitor v;
    return apply_visitors(prg, v);
}

xcml::xcml_program_node_ptr optimize_casts(xcml::xcml_program_node_ptr const& prg) {
    optimize_casts_visitor v1;
    optimize_base_casts_visitor v2;
    return apply_visitors(prg, v1, v2);
}
