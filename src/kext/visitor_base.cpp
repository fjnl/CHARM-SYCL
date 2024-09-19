#include <fmt/format.h>
#include "utils.hpp"
#include "visitor_impl.hpp"

clang::QualType visitor_base::expr_type(clang::Expr const* expr) {
    if (auto ce = clang::dyn_cast<clang::CastExpr>(expr)) {
        if (is_base_cast(ce)) {
            auto type = expr_type(ce->getSubExpr());
            return ast_.getPointerType(type);
        }

        if (ce->getCastKind() == clang::CK_UserDefinedConversion) {
            return expr_type(ce->getSubExpr());
        }

        if (is_integral_cast(ce) || is_ptr_to_ptr_cast(ce) ||
            ce->getCastKind() == clang::CK_ConstructorConversion) {
            return ce->getType();
        }

        if (ce->getCastKind() == clang::CK_LValueToRValue) {
            auto type = ce->getSubExpr()->getType();

            if (point_to_refenrece(info_, ce)) {
                type = type->getPointeeType();
            }

            return type;
        }

        if (is_noop_cast(ce)) {
            while (ce && is_noop_cast(ce)) {
                expr = ce->getSubExpr();
                ce = expr ? llvm::dyn_cast<clang::CastExpr>(expr) : nullptr;
            }
            return expr_type(expr);
        }

        not_supported(expr, ast_, "Not supported cast expression type");
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
        if (point_to_refenrece(info_, dre)) {
            type = dre->getDecl()->getType();
        }
    } else if (auto const* ce = clang::dyn_cast<clang::CallExpr>(expr)) {
        type = clang::dyn_cast<clang::FunctionDecl>(ce->getCalleeDecl())->getReturnType();
    }

    if (auto const* uop = clang::dyn_cast<clang::UnaryOperator>(expr);
        uop && uop->getOpcode() == clang::UO_Deref) {
        type = ast_.getLValueReferenceType(type);
    } else if (auto const* ase = clang::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
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

    return type;
}

xcml::function_call_ptr visitor_base::define_function(clang::Expr const* expr,
                                                      clang::FunctionDecl const* decl) {
    PUSH_CONTEXT(decl);

    if (decl->getQualifiedNameAsString() == "sycl::runtime::__charm_sycl_kernel") {
        auto const* call = clang::dyn_cast<clang::CallExpr>(expr);
        auto const* arg =
            clang::dyn_cast<clang::UnaryOperator>(call->getArg(0)->IgnoreImpCasts());
        auto const* ref = clang::dyn_cast<clang::DeclRefExpr>(arg->getSubExpr());

        info_.add_kernel(ref->getType()->getAsCXXRecordDecl());
    }

    auto const name = info_.encode_name(decl);

    if (auto desc = info_.find_func(name)) {
        return u::make_call(desc->addr);
    }

    bool visited = false;

    if (auto const* dc = decl->getDeclContext(); dc->isRecord()) {
        auto const* record = dc->getOuterLexicalRecordContext();

        if (auto const* cxx = clang::dyn_cast<clang::CXXRecordDecl>(record); cxx->isLambda()) {
            auto _set = info_.scoped_set_captures(cxx);
            visit_decl(nullptr, decl);
            visited = true;
        }
    }

    if (!visited) {
        visit_decl(nullptr, decl);
    }

    if (auto desc = info_.find_func(name)) {
        return u::make_call(desc->addr);
    }

    not_supported(decl, ast_);
}

xcml::function_call_ptr visitor_base::make_call_expr(clang::Expr const* expr,
                                                     xcml::expr_ptr this_ref,
                                                     bool this_is_ptr) {
    PUSH_CONTEXT(expr);

    auto const* call_expr = clang::dyn_cast<clang::CallExpr>(expr);
    auto const* ctor_expr = clang::dyn_cast<clang::CXXConstructExpr>(expr);

    if (!call_expr && !ctor_expr) {
        not_supported(expr, ast_);
    }

    auto const* decl = call_expr
                           ? clang::dyn_cast<clang::FunctionDecl>(call_expr->getCalleeDecl())
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

        if (type->isReferenceType()) {
            ref_args.insert(arg);
        } else if (type->isRecordType() && !type->isPointerType() && !type->isReferenceType()) {
            record_args.insert(arg);
        }
    }

    auto call = define_function(expr, decl);

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
        auto const* this_ = get_arg(expr, 0);

        if (!point_to_pointer(info_, this_) && !point_to_refenrece(info_, this_)) {
            ref_args.insert(this_);
        }
    }

    for (size_t i = 0; i < num_args(expr); i++) {
        auto const* arg = get_arg(expr, i);

        if (ref_args.count(arg)) {
            auto node = visit_expr_ref(arg);

            if (!point_to_pointer(info_, arg) && !point_to_refenrece(info_, arg)) {
                node = u::make_addr_of(node);
            }

            call->arguments.push_back(node);
        } else if (meth && i == 0) {
            call->arguments.push_back(visit_expr(arg));
        } else {
            call->arguments.push_back(visit_expr_val(arg));
        }
    }

    return call;
}
