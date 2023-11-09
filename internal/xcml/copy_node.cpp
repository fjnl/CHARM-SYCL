#include <unordered_map>
#include <pugixml.hpp>
#include <utils/naming.hpp>
#include <xcml_type.hpp>
#include <xcml_visitor.hpp>
#include "xcml_utils.hpp"

namespace {

struct copy_expr_visitor : xcml::visitor<copy_expr_visitor, xcml::expr_ptr>,
                           private utils::naming_utils {
    xcml::expr_ptr apply(xcml::xcml_program_node_ptr const& prg, xcml::expr_ptr const& node) {
        set_nextid(prg->gensym_id);
        auto const res = visit(node);
        prg->gensym_id = get_nextid();
        return res;
    }

    xcml::expr_ptr visit_unary_op(std::shared_ptr<xcml::unary_op> const& expr) {
        expr->expr = visit(expr->expr);
        return expr;
    }

    xcml::expr_ptr visit_binary_op(std::shared_ptr<xcml::binary_op> const& expr) {
        expr->lhs = visit(expr->lhs);
        expr->rhs = visit(expr->rhs);
        return expr;
    }

    xcml::expr_ptr visit_var_ref(xcml::var_ref_ptr const& expr) {
        rewrite_name(expr->name);
        return expr;
    }

    xcml::expr_ptr visit_member_ref(xcml::member_ref_ptr const& expr) {
        expr->value = visit(expr->value);
        return expr;
    }

    xcml::expr_ptr visit_func_addr(xcml::func_addr_ptr const& expr) {
        rewrite_name(expr->name);
        return expr;
    }

    xcml::expr_ptr visit_var_addr(xcml::var_addr_ptr const& expr) {
        rewrite_name(expr->name);
        return expr;
    }

    xcml::expr_ptr visit_array_addr(xcml::array_addr_ptr const& expr) {
        rewrite_name(expr->name);
        return expr;
    }

    xcml::expr_ptr visit_member_addr(xcml::member_addr_ptr const& expr) {
        expr->value = visit(expr->value);
        return expr;
    }

    xcml::expr_ptr visit_pointer_ref(xcml::pointer_ref_ptr const& expr) {
        expr->expr = visit(expr->expr);
        return expr;
    }

    xcml::expr_ptr visit_array_ref(xcml::array_ref_ptr const& expr) {
        expr->array = visit(expr->array);
        for (auto& index : expr->index) {
            index = visit(index);
        }
        return expr;
    }

    xcml::expr_ptr visit_member_array_ref(xcml::member_array_ref_ptr const& expr) {
        expr->value = visit(expr->value);
        return expr;
    }

    xcml::expr_ptr visit_function_call(xcml::function_call_ptr const& expr) {
        expr->function = visit(expr->function);
        for (auto& arg : expr->arguments) {
            arg = visit(arg);
        }
        return expr;
    }

    xcml::expr_ptr visit_int_constant(xcml::int_constant_ptr const& expr) {
        return expr;
    }

    xcml::expr_ptr visit_long_long_constant(xcml::long_long_constant_ptr const& expr) {
        return expr;
    }

    xcml::expr_ptr visit_string_constant(xcml::string_constant_ptr const& expr) {
        return expr;
    }

    xcml::expr_ptr visit_float_constant(xcml::float_constant_ptr const& expr) {
        return expr;
    }

    xcml::expr_ptr visit_cast_expr(xcml::cast_expr_ptr const& expr) {
        expr->value = visit(expr->value);
        return expr;
    }

private:
    void rewrite_name(std::string& name) const {
        if (auto const it = map_.find(name); it != map_.end()) {
            name = it->second;
        }
    }

    std::unordered_map<std::string, std::string> map_;
};

}  // namespace

namespace xcml {

expr_ptr copy_expr_impl(xcml_program_node_ptr const& prg, expr_ptr const& node) {
    pugi::xml_document doc;
    xcml::to_xml(doc, node);

    xcml::expr_ptr cloned;
    xcml::make_from_xml(*doc.begin(), cloned);

    copy_expr_visitor vis;
    return vis.apply(prg, cloned);
}

compound_stmt_ptr clone_compound(compound_stmt_ptr const& node) {
    pugi::xml_document doc;
    xcml::to_xml(doc, node);

    xcml::compound_stmt_ptr cloned;
    xcml::make_from_xml(*doc.begin(), cloned);

    return cloned;
}

}  // namespace xcml
