#pragma once

#include <unordered_set>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclVisitor.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <utils/naming.hpp>
#include <xcml_func.hpp>
#include <xcml_type.hpp>
#include <xcml_utils.hpp>
#include "error_trace.hpp"
#include "transform_info.hpp"
#include "utils.hpp"
#include "visitor.hpp"

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic warning "-Wall"
#    pragma GCC diagnostic warning "-Wextra"
#endif

namespace u = xcml::utils;

struct visitor_base {
    explicit visitor_base(transform_info& info, xcml::compound_stmt_ptr const& scope,
                          clang::Expr const* current_kernel,
                          clang::FunctionDecl const* current_func, bool current_is_kernel)
        : info_(info),
          ast_(info.ctx()),
          scope_(scope),
          current_kernel_(current_kernel),
          current_func_(current_func),
          current_is_kernel_(current_is_kernel) {}

    void visit_decl(clang::Decl const* decl, context const& = context()) {
        visit_decl(scope_, decl);
    }

    void visit_decl(xcml::compound_stmt_ptr const& scope, clang::Decl const* decl,
                    context const& = context());

    void visit_stmt(clang::Stmt const* stmt, context const& = context()) {
        visit_stmt(scope_, stmt);
    }

    void visit_stmt(xcml::compound_stmt_ptr const& scope, clang::Stmt const* stmt,
                    context const& = context());

    void descend_compound_stmt(xcml::compound_stmt_ptr const& scope,
                               clang::CompoundStmt const* stmt, xcml::expr_ptr* last_value,
                               context const& = context()) {
        for (auto stmt : stmt->body()) {
            if (auto decl = clang::dyn_cast<clang::DeclStmt>(stmt)) {
                PUSH_CONTEXT(decl);

                add_to_scope(scope, decl);
            } else if (auto const* expr = clang::dyn_cast<clang::Expr>(stmt)) {
                PUSH_CONTEXT(expr);

                auto val = visit_expr(scope, expr);
                if (last_value) {
                    *last_value = val;
                }
            } else {
                PUSH_CONTEXT(stmt);

                visit_stmt(scope, stmt);
            }
        }
    }

    xcml::compound_stmt_ptr visit_compound_stmt(xcml::compound_stmt_ptr const& scope,
                                                clang::CompoundStmt const* stmt,
                                                clang::FunctionDecl const* function,
                                                bool function_is_kernel,
                                                xcml::expr_ptr* last_value,
                                                context const& = context());

    xcml::compound_stmt_ptr visit_compound_stmt(clang::CompoundStmt const* stmt,
                                                clang::FunctionDecl const* function,
                                                bool function_is_kernel,
                                                xcml::expr_ptr* last_value = nullptr,
                                                context const& = context());

    xcml::compound_stmt_ptr visit_compound_stmt(clang::Stmt const* stmt,
                                                clang::FunctionDecl const* function,
                                                bool function_is_kernel,
                                                context const& = context());

    xcml::compound_stmt_ptr visit_compound_stmt(clang::Stmt const* stmt,
                                                context const& = context());

    xcml::compound_stmt_ptr visit_compound_stmt(xcml::compound_stmt_ptr const& scope,
                                                clang::CompoundStmt const* stmt,
                                                xcml::expr_ptr* last_value = nullptr,
                                                context const& = context());

    xcml::compound_stmt_ptr visit_compound_stmt(clang::CompoundStmt const* stmt,
                                                xcml::expr_ptr* last_value = nullptr,
                                                context const& = context());

    xcml::expr_ptr visit_expr(clang::Expr const* expr, context const& = context()) {
        return visit_expr(scope_, expr);
    }

    xcml::expr_ptr visit_expr(xcml::compound_stmt_ptr const& scope, clang::Expr const* expr,
                              context const& = context());

    xcml::expr_ptr visit_expr_val(clang::Expr const* expr, context const& = context()) {
        return visit_expr_val(scope_, expr);
    }

    xcml::expr_ptr visit_expr_val(xcml::compound_stmt_ptr const& scope, clang::Expr const* expr,
                                  context const& = context()) {
        auto const node = visit_expr(scope, expr);
        auto const type = expr_type(expr);

        if (!type.isNull() && type->isReferenceType()) {
            return deref(node);
        }
        return node;
    }

    xcml::function_call_ptr define_function(clang::FunctionDecl const* decl) {
        PUSH_CONTEXT(decl);

        auto const name = info_.encode_name(decl);

        if (auto desc = info_.find_func(name)) {
            return u::make_call(desc->addr);
        }

        visit_decl(nullptr, decl);

        if (auto desc = info_.find_func(name)) {
            return u::make_call(desc->addr);
        }

        not_supported(decl, ast_);
    }

protected:
    void push_expr(clang::Expr const* expr, xcml::expr_ptr const& node) {
        push_expr(scope_, expr, node);
    }

    template <class HasLoc>
    void push_expr(xcml::compound_stmt_ptr const& scope, HasLoc const* expr,
                   xcml::expr_ptr const& node) {
        if (expr) {
            auto& sm = ast_.getSourceManager();
            auto const loc = sm.getPresumedLoc(expr->getBeginLoc());

            if (loc.isValid()) {
                node->file = loc.getFilename();
                node->line = loc.getLine();
            } else {
                node->file = "<invalid>";
            }
        }
        u::push_expr(scope, node);
    }

    bool is_literal(xcml::expr_ptr const& expr) {
        return xcml::int_constant::is_a(expr) || xcml::float_constant::is_a(expr);
    }

    bool is_base_cast(clang::CastExpr const* expr) {
        return expr->getCastKind() == clang::CastKind::CK_UncheckedDerivedToBase ||
               expr->getCastKind() == clang::CastKind::CK_DerivedToBase;
    }

    bool is_noop_cast(clang::CastExpr const* expr) {
        return expr->getCastKind() == clang::CK_NoOp ||
               expr->getCastKind() == clang::CK_LValueToRValue ||
               expr->getCastKind() == clang::CK_ArrayToPointerDecay;
    }

    bool is_integral_cast(clang::CastExpr const* expr) {
        return expr->getCastKind() == clang::CK_IntegralCast ||
               expr->getCastKind() == clang::CK_IntegralToFloating ||
               expr->getCastKind() == clang::CK_FloatingCast ||
               expr->getCastKind() == clang::CK_IntegralToBoolean;
    }

    bool is_ptr_to_ptr_cast(clang::CastExpr const* expr) {
        return expr->getCastKind() == clang::CK_BitCast && expr->getType()->isPointerType() &&
               expr->getSubExpr()->getType()->isPointerType();
    }

    clang::QualType expr_type(clang::Expr const* expr) {
        if (auto ce = clang::dyn_cast<clang::CastExpr>(expr)) {
            if (is_base_cast(ce)) {
                auto type = expr_type(ce->getSubExpr());
                return ast_.getPointerType(type);
            }

            if (is_integral_cast(ce) || is_ptr_to_ptr_cast(ce)) {
                return ce->getType();
            }

            if (is_noop_cast(ce)) {
                while (ce && is_noop_cast(ce)) {
                    expr = ce->getSubExpr();
                    ce = expr ? llvm::dyn_cast<clang::CastExpr>(expr) : nullptr;
                }
                // continue below
            } else {
                not_supported(expr, ast_, "Not supported cast expression type");
            }
        }

        expr = expr->IgnoreImpCasts();

        auto type = expr->getType();
        auto const is_const = type.isConstQualified();

        if (auto const* me = clang::dyn_cast<clang::MemberExpr>(expr)) {
            type = me->getMemberDecl()->getType();
            if (is_const && !type.isConstQualified()) {
                type = ast_.getConstType(type);
            }
        } else if (auto const* dre = clang::dyn_cast<clang::DeclRefExpr>(expr)) {
            type = dre->getDecl()->getType();
        } else if (auto const* ce = clang::dyn_cast<clang::CallExpr>(expr)) {
            type = clang::dyn_cast<clang::FunctionDecl>(ce->getCalleeDecl())->getReturnType();
        }

        if (auto const* ase = clang::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
            if (type->isArrayType()) {
                type = ast_.getBaseElementType(type);
                type = ast_.getPointerType(type);
            } else {
                type = ast_.getLValueReferenceType(type);
            }
        } else if (type->isArrayType()) {
            type = ast_.getArrayDecayedType(type);

            if (is_const &&
                !(type.isConstQualified() ||
                  (type->isPointerType() && type->getPointeeType().isConstQualified()))) {
                type = ast_.getConstType(type);
            }
        }

        if (is_captured(expr)) {
            type = ast_.getPointerType(type);
        }

        return type;
    }

    xcml::pointer_ref_ptr deref(xcml::expr_ptr x) {
        auto deref = xcml::new_pointer_ref();
        deref->expr = x;
        return deref;
    }

    xcml::var_ref_ptr this_ref() {
        return u::make_var_ref(info_.nm().this_name());
    }

    bool is_kernel_functor() const {
        return current_kernel_ && !clang::isa<clang::LambdaExpr>(current_kernel_);
    }

    bool is_kernel_body_processing() const {
        return current_func_ && current_is_kernel_;
    }

    bool is_this_expr(clang::Expr const* expr) {
        return expr->isImplicitCXXThis() || clang::isa<clang::CXXThisExpr>(expr);
    }

    bool is_param(clang::Expr const* expr) {
        if (auto const* dr = clang::dyn_cast<clang::DeclRefExpr>(expr->IgnoreParenImpCasts())) {
            return clang::isa<clang::ParmVarDecl>(dr->getDecl());
        }
        return false;
    }

    bool is_captured(clang::Expr const* expr) {
        if (auto const* me = clang::dyn_cast<clang::MemberExpr>(expr)) {
            if (is_this_expr(me->getBase()) && is_kernel_functor() &&
                is_kernel_body_processing()) {
                return true;
            }

            return false;
        }

        if (auto const* dr = clang::dyn_cast<clang::DeclRefExpr>(expr)) {
            if (current_kernel_) {
                for (auto [vd, _] : captured_vars(info_, current_kernel_)) {
                    if (vd == dr->getDecl()) {
                        return true;
                    }
                }
                return false;
            }

            return false;
        }

        return false;
    }

    clang::Expr const* get_arg(clang::Expr const* expr, unsigned i) {
        PUSH_CONTEXT(expr);

        if (auto const* call = clang::dyn_cast<clang::CallExpr>(expr)) {
            return call->getArg(i);
        }
        if (auto const* ctor = clang::dyn_cast<clang::CXXConstructExpr>(expr)) {
            return ctor->getArg(i);
        }

        not_supported(expr, ast_);
    }

    size_t num_args(clang::Expr const* expr) {
        PUSH_CONTEXT(expr);

        if (auto const* call = clang::dyn_cast<clang::CallExpr>(expr)) {
            return call->getNumArgs();
        }
        if (auto const* ctor = clang::dyn_cast<clang::CXXConstructExpr>(expr)) {
            return ctor->getNumArgs();
        }

        not_supported(expr, ast_);
    }

    xcml::function_call_ptr make_call_expr(clang::Expr const* expr,
                                           xcml::expr_ptr this_ref = nullptr,
                                           bool this_is_ptr = false) {
        PUSH_CONTEXT(expr);

        // DUMP_COLOR(expr);
        auto const* call_expr = clang::dyn_cast<clang::CallExpr>(expr);
        auto const* ctor_expr = clang::dyn_cast<clang::CXXConstructExpr>(expr);

        if (!call_expr && !ctor_expr) {
            not_supported(expr, ast_);
        }

        auto const* decl =
            call_expr ? clang::dyn_cast<clang::FunctionDecl>(call_expr->getCalleeDecl())
                      : ctor_expr->getConstructor();
        auto const* mc = clang::dyn_cast<clang::CXXMemberCallExpr>(expr);
        auto const* oc = clang::dyn_cast<clang::CXXOperatorCallExpr>(expr);
        auto const* meth =
            oc ? clang::dyn_cast<clang::CXXMethodDecl>(oc->getCalleeDecl()) : nullptr;
        std::unordered_set<clang::Expr const*> ref_args, record_args;

        for (size_t i = 0; i < decl->getNumParams(); i++) {
            auto const arg = get_arg(expr, meth ? i + 1 : i);
            auto const pvd = decl->getParamDecl(i);
            auto const type = pvd->getType();

            if (type->isReferenceType() || clang::isa<clang::DecayedType>(*type)) {
                ref_args.insert(arg);
            } else if (type->isRecordType() && !type->isPointerType() &&
                       !type->isReferenceType()) {
                record_args.insert(arg);
            }
        }

        auto call = define_function(decl);

        if (mc) {
            auto const* obj = mc->getImplicitObjectArgument();
            this_ref = visit_expr(obj);

            auto const this_type = expr_type(obj);
            this_is_ptr = this_type->isPointerType() || this_type->isReferenceType();
        }
        if (this_ref) {
            if (!this_is_ptr) {
                this_ref = u::make_addr_of(this_ref);
            }
            call->arguments.push_back(this_ref);
        }
        if (meth) {
            ref_args.insert(get_arg(expr, 0));
        }

        for (size_t i = 0; i < num_args(expr); i++) {
            auto const* arg = get_arg(expr, i);

            if (ref_args.count(arg)) {
                auto node = visit_expr(arg);
                auto type = expr_type(arg);

                if (!type->isReferenceType() && !type->isPointerType()) {
                    node = u::make_addr_of(node);
                }

                call->arguments.push_back(node);
            } else if (record_args.count(arg)) {
                call->arguments.push_back(u::make_addr_of(visit_expr(arg)));
            } else {
                call->arguments.push_back(visit_expr_val(arg));
            }
        }

        return call;
    }

    void construct_inherit(xcml::compound_stmt_ptr const& scope, clang::Decl const* for_loc,
                           xcml::expr_ptr const& this_ref, bool this_is_ptr,
                           clang::CXXConstructorDecl const* ctor,
                           clang::FunctionDecl const* decl) {
        auto record = ctor->getParent();
        info_.check_device_copyable(record);

        if (ctor->isDefaultConstructor() && ctor->isTrivial()) {
            return;
        }

        auto call = define_function(ctor);
        xcml::expr_ptr this_expr = this_is_ptr ? this_ref : u::make_addr_of(this_ref);

        call->arguments.push_back(this_expr);
        for (auto param : decl->parameters()) {
            call->arguments.push_back(u::make_var_ref(info_.rename_sym(param)));
        }

        push_expr(scope, for_loc, call);
    }

    void construct(xcml::compound_stmt_ptr scope, xcml::expr_ptr const& this_ref,
                   bool this_is_ptr, clang::CXXConstructExpr const* expr,
                   xcml::var_ref_ptr const& ref) {
        auto ctor = expr->getConstructor();
        auto record = ctor->getParent();

        info_.check_device_copyable(record);

        if (ctor->isDefaultConstructor() && ctor->isTrivial()) {
            return;
        }

        if (ctor->isCopyOrMoveConstructor() && record->isTriviallyCopyable()) {
            auto asg = u::assign_expr(ref, visit_expr_val(expr->getArg(0)));
            push_expr(scope, expr, asg);
        } else {
            auto call = make_call_expr(expr, this_ref, this_is_ptr);
            push_expr(scope, expr, call);
        }
    }

    bool is_runtime_extern(clang::FunctionDecl const* decl) {
        return has_annotate(decl, "charm_sycl_extern_runtime");
    }

    void add_to_scope(xcml::compound_stmt_ptr scope, clang::DeclStmt const* stmt) {
        PUSH_CONTEXT(stmt);

        for (auto decl : stmt->decls()) {
            PUSH_CONTEXT(decl);

            visit_decl(scope, decl);
        }
    }

    xcml::expr_ptr visit_apsint(llvm::APSInt const& val) {
        if (val.isSignedIntN(64)) {
            return u::lit(val.getSExtValue());
        } else if (val.isIntN(64)) {
            return u::lit(val.getExtValue());
        }
        return nullptr;
    }

    transform_info& info_;
    clang::ASTContext& ast_;
    xcml::compound_stmt_ptr scope_;
    clang::Expr const* current_kernel_;
    clang::FunctionDecl const* current_func_;
    bool current_is_kernel_;
};

template <class Derived, class RetTy>
struct decl_visitor_base : clang::ConstDeclVisitor<Derived, RetTy>, visitor_base {
    using visitor_base::visitor_base;

    void Visit(clang::Decl const* decl, context const& = context()) {
        return clang::ConstDeclVisitor<Derived, RetTy>::Visit(decl);
    }

    void VisitDecl(clang::Decl const* decl, context const& = context()) {
        not_supported(decl, info_.ctx());
    }
};

template <class Derived, class RetTy, class Arg = void>
struct stmt_visitor_base : clang::ConstStmtVisitor<Derived, RetTy, Arg, context const&>,
                           visitor_base {
    using visitor_base::visitor_base;

    RetTy Visit(clang::Stmt const* stmt, Arg const& arg = Arg(),
                context const& ctx = context()) {
        return clang::ConstStmtVisitor<Derived, RetTy, Arg, context const&>::Visit(stmt, arg,
                                                                                   ctx);
    }

    RetTy VisitStmt(clang::Stmt const* stmt, Arg const& = Arg(), context const& = context()) {
        not_supported(stmt, info_.ctx());
    }

    RetTy VisitExpr(clang::Expr const* expr, Arg const& = Arg(), context const& = context()) {
        not_supported(expr, info_.ctx());
    }
};

template <class Derived, class RetTy>
struct stmt_visitor_base<Derived, RetTy, void>
    : clang::ConstStmtVisitor<Derived, RetTy, context const&>, visitor_base {
    using visitor_base::visitor_base;

    RetTy Visit(clang::Stmt const* stmt, context const& ctx = context()) {
        return clang::ConstStmtVisitor<Derived, RetTy, context const&>::Visit(stmt, ctx);
    }

    RetTy VisitStmt(clang::Stmt const* stmt, context const& = context()) {
        not_supported(stmt, info_.ctx());
    }

    RetTy VisitExpr(clang::Expr const* expr, context const& = context()) {
        not_supported(expr, info_.ctx());
    }
};
