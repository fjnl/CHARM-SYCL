#include "visitor_impl.hpp"

#define EXTRA_ARGS xcml::expr_ptr* = nullptr, context const& = context()

struct stmt_visitor final : stmt_visitor_base<stmt_visitor, void, xcml::expr_ptr*> {
    using stmt_visitor_base::stmt_visitor_base;

    void VisitCompoundStmt(clang::CompoundStmt const* compound,
                           xcml::expr_ptr* last_value = nullptr, context const& = context()) {
        PUSH_CONTEXT(compound);

        auto node = xcml::new_compound_stmt();

        descend_compound_stmt(node, compound, last_value);

        if (scope_) {
            add(node);
        } else {
            scope_ = node;
        }
    }

    void VisitIfStmt(clang::IfStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        if (stmt->isConstexpr()) {
            if (auto const* ce = clang::dyn_cast<clang::ConstantExpr>(stmt->getCond())) {
                if (!ce->getResultAsAPSInt() == 0) {
                    visit_stmt(stmt->getThen());
                    return;
                } else {
                    if (stmt->getElse()) {
                        visit_stmt(stmt->getElse());
                    }
                    return;
                }
            }

            not_supported(stmt->getCond(), ast_, "expected ConstantExpr");
        }

        auto node = xcml::new_if_stmt();

        node->condition = visit_expr(stmt->getCond());
        node->then = visit_compound_stmt(stmt->getThen());
        if (stmt->getElse()) {
            node->else_ = visit_compound_stmt(stmt->getElse());
        }

        add(node);
    }

    void VisitForStmt(clang::ForStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        auto node = xcml::new_for_stmt();
        xcml::expr_ptr cond_var;

        if (auto init = stmt->getInit()) {
            visit_stmt(init);
        }
        if (auto cond = stmt->getCond()) {
            cond_var = visit_expr(cond);
            node->condition = cond_var;
        }
        if (auto body = stmt->getBody()) {
            node->body = visit_compound_stmt(body);
        }
        if (auto inc = stmt->getInc()) {
            visit_expr(node->body, inc);
        }
        if (auto cond = stmt->getCond()) {
            auto asg = u::assign_expr(cond_var, visit_expr(node->body, cond));
            u::push_expr(node->body, asg);
        }

        add(node);
    }

    void VisitReturnStmt(clang::ReturnStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);
        assert(current_func_ != nullptr);

        auto node = xcml::new_return_stmt();
        auto value = stmt->getRetValue();

        if (value) {
            auto ret_ty = current_func_->getReturnType();

            if (ret_ty->isPointerType() || ret_ty->isReferenceType()) {
                auto ty = expr_type(value);
                if (ty->isPointerType() || ty->isReferenceType()) {
                    node->value = visit_expr(value);
                } else {
                    node->value = u::make_addr_of(visit_expr_val(value));
                }
            } else {
                node->value = visit_expr_val(value);
            }
        }

        add(node);
    }

    void VisitDeclStmt(clang::DeclStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        for (auto const* decl : stmt->decls()) {
            visit_decl(decl);
        }
    }

    void VisitSwitchStmt(clang::SwitchStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        auto node = u::new_switch_stmt();

        node->value = visit_expr_val(stmt->getCond());
        node->body = visit_compound_stmt(stmt->getBody());

        add(node);
    }

    void VisitCaseStmt(clang::CaseStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        auto node = u::new_case_label_stmt();
        auto const* lhs = stmt->getLHS();

        if (auto ce = clang::dyn_cast<clang::ConstantExpr>(lhs)) {
            auto val = ce->getResultAsAPSInt();

            node->value = visit_apsint(val);
            if (!node->value) {
                not_supported(lhs, ast_, "integer overflow");
            }
        } else {
            not_supported(stmt, ast_, "Not supported label");
        }

        add(node);

        visit_stmt(stmt->getSubStmt());
    }

    void VisitDefaultStmt(clang::DefaultStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        add(u::new_default_stmt());

        visit_stmt(stmt->getSubStmt());
    }

    void VisitBreakStmt(clang::BreakStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        add(u::new_break_stmt());
    }

    xcml::compound_stmt_ptr get_scope() const {
        return scope_;
    }

private:
    void add(xcml::stmt_ptr const& stmt, context const& = context()) {
        if (!scope_) {
            not_supported(ast_, "Assetion Failed: scope is NULL");
        }
        u::push_stmt(scope_, stmt);
    }
};

xcml::compound_stmt_ptr visit_compound_stmt(transform_info& info,
                                            clang::CompoundStmt const* stmt,
                                            clang::Expr const* kernel,
                                            clang::FunctionDecl const* function,
                                            bool function_is_kernel, context const&) {
    stmt_visitor vis(info, nullptr, kernel, function, function_is_kernel);
    vis.Visit(stmt);
    return vis.get_scope();
}

xcml::compound_stmt_ptr visit_compound_stmt(transform_info& info, clang::Stmt const* stmt,
                                            clang::Expr const* kernel,
                                            clang::FunctionDecl const* function,
                                            bool function_is_kernel, context const& ctx) {
    if (auto c = clang::dyn_cast<clang::CompoundStmt>(stmt)) {
        return visit_compound_stmt(info, c, kernel, function, function_is_kernel, ctx);
    }

    not_supported(stmt, info.ctx(), "visit_compound_stmt(): not a compound statement");
}

void visitor_base::visit_stmt(xcml::compound_stmt_ptr const& scope, clang::Stmt const* stmt,
                              context const&) {
    if (auto const* expr = clang::dyn_cast<clang::Expr>(stmt)) {
        visit_expr(scope, expr);
    } else {
        stmt_visitor vis(info_, scope, current_kernel_, current_func_, current_is_kernel_);
        vis.Visit(stmt);
    }
}

xcml::compound_stmt_ptr visitor_base::visit_compound_stmt(clang::CompoundStmt const* stmt,
                                                          clang::FunctionDecl const* function,
                                                          bool function_is_kernel,
                                                          xcml::expr_ptr* last_value,
                                                          context const&) {
    stmt_visitor vis(info_, nullptr, current_kernel_, function, function_is_kernel);
    vis.Visit(stmt, last_value);
    return vis.get_scope();
}

xcml::compound_stmt_ptr visitor_base::visit_compound_stmt(clang::Stmt const* stmt,
                                                          clang::FunctionDecl const* function,
                                                          bool function_is_kernel,
                                                          context const&) {
    if (auto c = clang::dyn_cast<clang::CompoundStmt>(stmt)) {
        return visit_compound_stmt(c, function, function_is_kernel);
    }

    not_supported(stmt, ast_, "visiting not a compound statement");
}

xcml::compound_stmt_ptr visitor_base::visit_compound_stmt(clang::CompoundStmt const* stmt,
                                                          xcml::expr_ptr* last_value,
                                                          context const&) {
    return visit_compound_stmt(stmt, current_func_, current_is_kernel_, last_value);
}

xcml::compound_stmt_ptr visitor_base::visit_compound_stmt(clang::Stmt const* stmt,
                                                          context const&) {
    if (auto c = clang::dyn_cast<clang::CompoundStmt>(stmt)) {
        return visit_compound_stmt(c);
    }

    auto c = u::new_compound_stmt();

    if (auto const* expr = clang::dyn_cast<clang::Expr>(stmt)) {
        visit_expr(c, expr);
    } else {
        visit_stmt(c, stmt);
    }

    return c;
}
