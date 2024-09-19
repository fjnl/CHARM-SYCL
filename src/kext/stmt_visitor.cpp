#include "visitor_impl.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#    pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-parameter"
#    pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif

#include <clang/AST/Attr.h>
#include <clang/AST/ParentMapContext.h>

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

#define EXTRA_ARGS xcml::expr_ptr* = nullptr, context const& = context()

struct stmt_visitor final : stmt_visitor_base<stmt_visitor, void, xcml::expr_ptr*> {
    using stmt_visitor_base::stmt_visitor_base;

    void VisitAttributedStmt(clang::AttributedStmt const* stmt, EXTRA_ARGS) {
        visit_stmt(stmt->getSubStmt());
    }

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

    bool is_simple_expr(clang::Expr const* expr, bool nested, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        expr = expr->IgnoreImpCasts();

        if (nested) {
            return clang::isa<clang::DeclRefExpr, clang::IntegerLiteral,
                              clang::FloatingLiteral>(expr);
        }

        if (auto const* unary = clang::dyn_cast<clang::UnaryOperator>(expr)) {
            return is_simple_expr(unary->getSubExpr(), true);
        } else if (auto const* binary = clang::dyn_cast<clang::BinaryOperator>(expr)) {
            return is_simple_expr(binary->getLHS(), true) &&
                   is_simple_expr(binary->getRHS(), true);
        } else if (auto const* call = clang::dyn_cast<clang::CallExpr>(expr)) {
            for (auto const* arg : call->arguments()) {
                if (!is_simple_expr(arg, true)) {
                    return false;
                }
            }

            return true;
        }

        return is_simple_expr(expr, true);
    }

    xcml::expr_ptr visit_simple_expr(clang::Expr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        if (is_simple_expr(expr, false)) {
            return visit_expr_direct(expr);
        }

        return nullptr;
    }

    void VisitForStmt_2(clang::ForStmt const* stmt, llvm::StringRef anno, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        auto node = xcml::new_for_stmt();

        /*
         * for (TYPE VAR = SIMPLE_INIT_EXPR; SIMPLE_COND_EXPR; VAR = SIMPLE_INC_EXPR)
         */

        auto const* vd = clang::dyn_cast<clang::VarDecl>(
            clang::dyn_cast<clang::DeclStmt>(stmt->getInit())->getSingleDecl());
        auto const type = vd->getType();
        auto const var =
            u::add_local_var(scope_, info_.define_type(type), info_.rename_sym(vd));
        auto const* init = vd->getInit();
        auto const* cond = stmt->getCond();
        auto const* inc = stmt->getInc();

        node->init = u::assign_expr(var, visit_simple_expr(init));
        if (!node->init) {
            not_supported(init, ast_, "init: not a simple expression");
        }

        node->condition = visit_simple_expr(cond);
        if (!node->init) {
            not_supported(cond, ast_, "cond: not a simple expression");
        }

        if (auto const* asg = clang::dyn_cast<clang::BinaryOperator>(inc);
            asg && asg->getOpcode() == clang::BO_Assign) {
            auto asg_node = u::new_assign_expr();

            asg_node->lhs = visit_simple_expr(asg->getLHS());
            if (!asg_node->lhs) {
                not_supported(asg->getLHS(), ast_, "inc/lhs: not a simple expression");
            }

            asg_node->rhs = visit_simple_expr(asg->getRHS());
            if (!asg_node->rhs) {
                not_supported(asg->getRHS(), ast_, "inc/rhs: not a simple expression");
            }

            node->iter = asg_node;
        }

        if (auto const* body = stmt->getBody()) {
            node->body = visit_compound_stmt(body);
        }

        if (anno.contains(" top ")) {
            llvm::APInt v;
            int collapse = 1;

            if (anno.rsplit(' ').second.getAsInteger(10, v)) {
                collapse = v.getZExtValue();
            }

            node->pragma.push_back(
                u::make_pragma(fmt::format("omp parallel for collapse({})", collapse)));
        } else {
            // node->pragma.push_back(u::make_pragma("loop"));
        }

        add(node);
    }

    void VisitForStmt(clang::ForStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        if (auto const* init = stmt->getInit()) {
            if (auto const* ds = clang::dyn_cast<clang::DeclStmt>(init); ds->isSingleDecl()) {
                if (auto const* vd = clang::dyn_cast<clang::VarDecl>(ds->getSingleDecl())) {
                    for (auto const* attr : vd->attrs()) {
                        if (auto const* anno = clang::dyn_cast<clang::AnnotateAttr>(attr)) {
                            if (anno->getAnnotation().startswith("charm_sycl_parallel_for ")) {
                                return VisitForStmt_2(stmt, anno->getAnnotation());
                            }
                        }
                    }
                }
            }
        }

        auto node = xcml::new_for_stmt();
        xcml::expr_ptr cond_var;

        if (auto init = stmt->getInit()) {
            visit_stmt(init);
        }
        if (auto cond = stmt->getCond()) {
            cond_var = visit_expr(cond);
            node->condition = cond_var;

            info_.add_for_condition(stmt, cond_var);
        }
        if (auto body = stmt->getBody()) {
            node->body = visit_compound_stmt(body);
        }
        if (auto inc = stmt->getInc()) {
            visit_expr(node->body, inc);
        }
        if (auto cond = stmt->getCond()) {
            auto asg = u::assign_expr(cond_var, visit_expr(node->body, cond));
            push_expr(node->body, cond, asg);
        }

        add(node);
    }

    void VisitWhileStmt(clang::WhileStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        auto node = xcml::new_while_stmt();
        xcml::expr_ptr cond_var;

        if (auto cond = stmt->getCond()) {
            cond_var = visit_expr_val(cond);
            node->condition = cond_var;
        }
        if (auto body = stmt->getBody()) {
            node->body = visit_compound_stmt(body);
        }
        if (auto cond = stmt->getCond()) {
            auto asg = u::assign_expr(cond_var, visit_expr_val(node->body, cond));
            push_expr(node->body, cond, asg);
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

            if (ret_ty->isReferenceType() && expr_type(value)->isReferenceType()) {
                if (auto const* uo =
                        clang::dyn_cast<clang::UnaryOperator>(value->IgnoreImpCasts())) {
                    if (uo->getOpcode() == clang::UO_Deref) {
                        node->value = u::make_addr_of(visit_expr(value));
                    }
                } else {
                    node->value = visit_expr(value);
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

    void VisitContinueStmt(clang::ContinueStmt const* stmt, EXTRA_ARGS) {
        PUSH_CONTEXT(stmt);

        auto& pmc = ast_.getParentMapContext();
        clang::Stmt const* p = stmt;

        for (; p;) {
            auto parents = pmc.getParents(*p);

            if (parents.empty()) {
                p = nullptr;
            } else {
                if (auto const* for_ = parents[0].get<clang::ForStmt>()) {
                    p = for_;
                    break;
                } else if (auto const* while_ = parents[0].get<clang::WhileStmt>()) {
                    p = while_;
                    break;
                } else if (auto const* stmt = parents[0].get<clang::Stmt>()) {
                    p = stmt;
                } else {
                    p = nullptr;
                }
            }
        }

        if (!p) {
            not_supported(stmt, ast_, "cannot find the parent loop");
        }

        if (auto const* for_ = clang::dyn_cast<clang::ForStmt>(p)) {
            visit_expr(for_->getInc());

            if (auto cond = for_->getCond()) {
                auto cond_var = info_.lookup_for_condition(for_);

                push_expr(cond, u::assign_expr(cond_var, visit_expr(cond)));
            }
        }

        add(u::new_continue_stmt());
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

xcml::compound_stmt_ptr visitor_base::visit_compound_stmt(clang::CompoundStmt const* stmt,
                                                          clang::Expr const* kernel,
                                                          clang::FunctionDecl const* function,
                                                          context const&) {
    stmt_visitor vis(info_, nullptr, kernel, function, true);
    vis.Visit(stmt);
    return vis.get_scope();
}
