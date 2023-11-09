#include "visitor_impl.hpp"

#define EXTRA_ARGS bool = false, context const& = context()

struct expr_visitor final : stmt_visitor_base<expr_visitor, xcml::expr_ptr, bool> {
    using stmt_visitor_base::stmt_visitor_base;

    xcml::expr_ptr create_assign_expr(clang::Expr const* expr, clang::Expr const* lhs,
                                      clang::Expr const* rhs) {
        auto node = u::new_assign_expr();
        node->lhs = asg_op_lhs(lhs);
        node->rhs = visit_expr_val(rhs);
        push_expr(expr, node);
        return node->lhs;
    }

    xcml::expr_ptr VisitBinAssign(clang::BinaryOperator const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);
        return create_assign_expr(expr, expr->getLHS(), expr->getRHS());
    }

#define BIN_OP(fn, clang_type, xcml_type)                                              \
private:                                                                               \
    xcml::expr_ptr create_##xcml_type(clang::Expr const* expr, clang::Expr const* lhs, \
                                      clang::Expr const* rhs, EXTRA_ARGS) {            \
        auto node = u::new_##xcml_type();                                              \
        node->lhs = visit_expr_val(lhs);                                               \
        node->rhs = visit_expr_val(rhs);                                               \
        return add(expr, node);                                                        \
    }                                                                                  \
                                                                                       \
public:                                                                                \
    xcml::expr_ptr Visit##fn(clang::clang_type const* expr, EXTRA_ARGS) {              \
        PUSH_CONTEXT(expr);                                                            \
        return create_##xcml_type(expr, expr->getLHS(), expr->getRHS());               \
    }

#define ASG_OP(fn, clang_type, xcml_type)                                 \
    xcml::expr_ptr Visit##fn(clang::clang_type const* expr, EXTRA_ARGS) { \
        PUSH_CONTEXT(expr);                                               \
        auto node = u::new_##xcml_type();                                 \
        node->lhs = asg_op_lhs(expr->getLHS());                           \
        node->rhs = visit_expr_val(expr->getRHS());                       \
        push_expr(expr, node);                                            \
        return node->lhs;                                                 \
    }

#define UNARY_OP(fn, clang_type, xcml_type)                                      \
    xcml::expr_ptr Visit##fn(clang::clang_type const* expr, bool direct = false, \
                             context const& = context()) {                       \
        PUSH_CONTEXT(expr);                                                      \
        auto node = u::new_##xcml_type();                                        \
        node->expr = visit_expr_val(expr->getSubExpr());                         \
        return direct ? node : add(expr, node);                                  \
    }

    ASG_OP(BinAddAssign, CompoundAssignOperator, asg_plus_expr)
    ASG_OP(BinDivAssign, CompoundAssignOperator, asg_div_expr)
    ASG_OP(BinMulAssign, CompoundAssignOperator, asg_mul_expr)
    ASG_OP(BinSubAssign, CompoundAssignOperator, asg_minus_expr)
    BIN_OP(BinAdd, BinaryOperator, plus_expr)
    BIN_OP(BinAnd, BinaryOperator, bit_and_expr)
    BIN_OP(BinDiv, BinaryOperator, div_expr)
    BIN_OP(BinEQ, BinaryOperator, log_eq_expr)
    BIN_OP(BinGE, BinaryOperator, log_ge_expr)
    BIN_OP(BinGT, BinaryOperator, log_gt_expr)
    BIN_OP(BinLAnd, BinaryOperator, log_and_expr)
    BIN_OP(BinLE, BinaryOperator, log_le_expr)
    BIN_OP(BinLOr, BinaryOperator, log_or_expr)
    BIN_OP(BinLT, BinaryOperator, log_lt_expr)
    BIN_OP(BinMul, BinaryOperator, mul_expr)
    BIN_OP(BinNE, BinaryOperator, log_neq_expr)
    BIN_OP(BinOr, BinaryOperator, bit_or_expr)
    BIN_OP(BinRem, BinaryOperator, mod_expr)
    BIN_OP(BinSub, BinaryOperator, minus_expr)
    BIN_OP(BinXor, BinaryOperator, bit_xor_expr)
    UNARY_OP(UnaryAddrOf, UnaryOperator, addr_of_expr)
    UNARY_OP(UnaryDeref, UnaryOperator, pointer_ref)
    UNARY_OP(UnaryLNot, UnaryOperator, log_not_expr)
    UNARY_OP(UnaryMinus, UnaryOperator, unary_minus_expr)
    UNARY_OP(UnaryPlus, UnaryOperator, unary_plus_expr)
    UNARY_OP(UnaryPostDec, UnaryOperator, post_decr_expr)
    UNARY_OP(UnaryPostInc, UnaryOperator, post_incr_expr)
    UNARY_OP(UnaryPreDec, UnaryOperator, pre_decr_expr)
    UNARY_OP(UnaryPreInc, UnaryOperator, pre_incr_expr)

#undef BIN_OP
#undef UNARY_OP

    xcml::expr_ptr VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr const* expr,
                                            EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        if (auto const* method = clang::dyn_cast<clang::CXXMethodDecl>(expr->getCalleeDecl());
            method &&
            (method->isCopyAssignmentOperator() || method->isMoveAssignmentOperator())) {
            auto const type = method->getThisObjectType();

            if (type.isTriviallyCopyableType(ast_)) {
                return create_assign_expr(expr, expr->getArg(0), expr->getArg(1));
            }
        }

        return add(expr, make_call_expr(expr));
    }

    xcml::expr_ptr VisitCallExpr(clang::CallExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        return add(expr, make_call_expr(expr));
    }

    xcml::expr_ptr VisitExprWithCleanups(clang::ExprWithCleanups const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        if (expr->getNumObjects() == 0) {
            return Visit(expr->getSubExpr());
        }

        // TODO:
        not_supported(expr, ast_);
    }

    xcml::expr_ptr VisitArraySubscriptExpr(clang::ArraySubscriptExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        auto add_expr = u::new_plus_expr();
        auto cast_expr = u::new_cast_expr();

        add_expr->lhs = visit_expr(expr->getBase());
        add_expr->rhs = visit_expr_val(expr->getIdx());

        // cast_expr->type = info_.define_type(ast_.VoidPtrTy);
        cast_expr->value = add_expr;

        return add(expr, cast_expr, &cast_expr->type);
    }

    xcml::expr_ptr VisitCXXFunctionalCastExpr(clang::CXXFunctionalCastExpr const* expr,
                                              EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        if (is_noop_cast(expr) || expr->getCastKind() == clang::CK_ConstructorConversion) {
            return Visit(expr->getSubExpr());
        }

        not_supported(expr, ast_, expr->getCastKindName());
    }

    xcml::expr_ptr VisitCastExpr(clang::CastExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        if (is_integral_cast(expr) || is_ptr_to_ptr_cast(expr)) {
            return do_integral_cast(expr);
        }

        if (is_base_cast(expr)) {
            return do_base_cast(expr);
        }

        if (is_noop_cast(expr)) {
            return Visit(expr->getSubExpr());
        }

        not_supported(expr, ast_);
    }

    xcml::expr_ptr VisitMemberExpr(clang::MemberExpr const* expr, bool direct = false,
                                   context const& = context()) {
        PUSH_CONTEXT(expr);

        if (is_this_expr(expr->getBase()) && is_kernel_functor() &&
            is_kernel_body_processing()) {
            return u::make_var_ref(info_.rename_sym(expr->getMemberDecl()));
        }

        auto ref = xcml::new_member_ref();

        auto base_type = expr_type(expr->getBase());
        auto base_expr = visit_expr(expr->getBase());

        if (!is_param(expr->getBase()) && !base_type->isPointerType() &&
            !base_type->isReferenceType()) {
            if (auto var = xcml::var_ref::dyncast(base_expr)) {
                base_expr = u::make_addr_of(var);
            }
        }

        ref->value = base_expr;
        ref->member = expr->getMemberNameInfo().getAsString();

        return direct ? ref : add(expr, ref);
    }

    xcml::expr_ptr VisitCXXThisExpr(clang::CXXThisExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        return this_ref();
    }

    xcml::expr_ptr VisitCXXConstructExpr(clang::CXXConstructExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        auto type = expr->getType();
        type.removeLocalConst();

        auto var =
            u::add_local_var(scope_, info_.define_type(type), info_.nm().gen_var("temp"));

        construct(scope_, var, false, expr, var);

        return var;
    }

    xcml::expr_ptr VisitDeclRefExpr(clang::DeclRefExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        auto const* vd = expr->getDecl();

        if (auto const* ecd = clang::dyn_cast<clang::EnumConstantDecl>(vd)) {
            return visit_apsint(ecd->getInitVal());
        }

        if (expr->isNonOdrUse() == clang::NOUR_Constant) {
            if (auto var = clang::dyn_cast<clang::VarDecl>(vd)) {
                auto init = var->getAnyInitializer(var);

                if (!clang::isa<clang::IntegerLiteral, clang::FloatingLiteral>(
                        init->IgnoreCasts())) {
                    not_supported(vd, ast_);
                }

                return Visit(init);
            }

            not_supported(vd, ast_);
        }

        auto x = xcml::new_var_ref();
        x->name = info_.rename_sym(vd);

        return x;
    }

    xcml::expr_ptr VisitMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr const* expr,
                                                 EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        auto node = Visit(expr->getSubExpr());

        if (is_literal(node)) {
            node = add(expr->getSubExpr(), node);
        }

        return node;
    }

    xcml::expr_ptr VisitCXXTemporaryObjectExpr(clang::CXXTemporaryObjectExpr const* expr,
                                               EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        auto const& type = info_.define_type(expr->getType());
        auto var = u::add_local_var(scope_, type, info_.nm().gen_var("temp"));

        construct(scope_, var, false, expr, var);

        return var;
    }

    xcml::expr_ptr VisitSubstNonTypeTemplateParmExpr(
        clang::SubstNonTypeTemplateParmExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        return Visit(expr->getReplacement());
    }

    xcml::expr_ptr VisitIntegerLiteral(clang::IntegerLiteral const* lit, EXTRA_ARGS) {
        PUSH_CONTEXT(lit);

        auto x = xcml::new_int_constant();
        auto type = lit->getType();
        auto const& value = lit->getValue();

        if (type->isSignedIntegerType()) {
            x->value = std::to_string(value.getSExtValue());
        } else {
            x->value = std::to_string(value.getZExtValue());
        }
        x->type = info_.define_type(type);

        return x;
    }

    xcml::expr_ptr VisitFloatingLiteral(clang::FloatingLiteral const* lit, EXTRA_ARGS) {
        PUSH_CONTEXT(lit);

        std::array<char, 100> buff;
        lit->getValue().convertToHexString(buff.data(), 0, false,
                                           llvm::APFloatBase::roundingMode::NearestTiesToEven);

        auto f = xcml::new_float_constant();
        f->type = info_.define_type(lit->getType());
        f->value = buff.data();

        return f;
    }

    xcml::expr_ptr VisitUnaryExprOrTypeTraitExpr(clang::UnaryExprOrTypeTraitExpr const* expr,
                                                 EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        switch (expr->getKind()) {
            case clang::UETT_SizeOf: {
                auto const sz = ast_.getTypeSize(expr->getArgumentType());

                if (!(sz % 8 == 0)) {
                    not_supported(expr, ast_, "assertion failed: sz % 8 == 0");
                }

                return u::lit(sz / 8);
            }

            default:
                not_supported(expr, ast_);
        }
    }

    xcml::expr_ptr VisitParenExpr(clang::ParenExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        return Visit(expr->getSubExpr());
    }

    xcml::expr_ptr VisitStmtExpr(clang::StmtExpr const* expr, EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        xcml::expr_ptr val;

        descend_compound_stmt(scope_, expr->getSubStmt(), &val);

        return val;
    }

    xcml::expr_ptr VisitConditionalOperator(clang::ConditionalOperator const* expr,
                                            EXTRA_ARGS) {
        PUSH_CONTEXT(expr);

        auto node = u::new_cond_expr();

        node->cond = visit_expr_val(expr->getCond());
        node->true_ = visit_expr_val(expr->getTrueExpr());
        node->false_ = visit_expr_val(expr->getFalseExpr());

        return node;
    }

private:
    xcml::expr_ptr add(clang::Expr const* expr, xcml::expr_ptr const& node,
                       std::string* type_out = nullptr) {
        if (expr->getType()->isVoidType()) {
            if (type_out) {
                *type_out = "void";
            }
            push_expr(expr, node);
            return nullptr;
        }

        return add(expr_type(expr), node, type_out);
    }

    xcml::expr_ptr add(clang::QualType type, xcml::expr_ptr const& node,
                       std::string* type_out = nullptr) {
        if (type->isVoidType()) {
            if (type_out) {
                *type_out = "void";
            }
            push_expr(nullptr, node);
            return nullptr;
        }

        if (!scope_) {
            not_supported(ast_, "Assertion Failed: scope_ is nullptr");
        }

        if (type.isLocalConstQualified() && !type->isReferenceType() &&
            !type->isPointerType()) {
            type.removeLocalConst();
        }

        auto const& type_name = info_.define_type(type);

        if (type_out) {
            *type_out = type_name;
        }

        return u::add_local_var(scope_, type_name, info_.nm().gen_var("temp"), node);
    }

    xcml::expr_ptr asg_op_lhs(clang::Expr const* lhs) {
        // FIXME: must be more general
        PUSH_CONTEXT(lhs);

        if (auto const* me = clang::dyn_cast<clang::MemberExpr>(lhs)) {
            return VisitMemberExpr(me, true);
        }

        if (auto const* ref = clang::dyn_cast<clang::DeclRefExpr>(lhs)) {
            auto node = VisitDeclRefExpr(ref);
            auto lhs_type = expr_type(ref);

            if (lhs_type->isReferenceType()) {
                node = deref(node);
            }
            return node;
        }

        if (auto const* una = clang::dyn_cast<clang::UnaryOperator>(lhs)) {
            if (una->getOpcode() == clang::UO_Deref) {
                return VisitUnaryDeref(una, true);
            }
        }

        return visit_expr_val(lhs);
    }

    xcml::expr_ptr do_integral_cast(clang::CastExpr const* expr) {
        PUSH_CONTEXT(expr);

        auto type = expr->getType();
        type.removeLocalConst();

        auto const node = xcml::new_cast_expr();
        node->value = visit_expr_val(expr->getSubExpr());
        node->type = info_.define_type(type);

        return add(type, node);
    }

    xcml::expr_ptr do_base_cast(clang::CastExpr const* expr) {
        PUSH_CONTEXT(expr);

        auto type = expr->getType();
        auto sub_type = expr_type(expr->getSubExpr());

        auto node = xcml::new_cast_expr();
        node->value = visit_expr(expr->getSubExpr());
        node->to_base = true;

        if (is_this_expr(expr->getSubExpr())) {
            // nop.
        } else if (is_captured(expr->getSubExpr())) {
            type = ast_.getPointerType(type);
        } else if (!type->isPointerType()) {
            type = ast_.getPointerType(type);
        }

        if (!sub_type->isPointerType() && !sub_type->isReferenceType()) {
            node->value = u::make_addr_of(node->value);
        }

        node->type = info_.define_type(type);

        return add(type, node);
    }
};

xcml::expr_ptr visitor_base::visit_expr(xcml::compound_stmt_ptr const& scope,
                                        clang::Expr const* expr, context const&) {
    expr_visitor vis(info_, scope, current_kernel_, current_func_, current_is_kernel_);
    return vis.Visit(expr);
}
