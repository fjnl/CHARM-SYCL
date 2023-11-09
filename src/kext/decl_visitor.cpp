#include "visitor_impl.hpp"

struct decl_visitor : decl_visitor_base<decl_visitor, void> {
    using decl_visitor_base::decl_visitor_base;

    void VisitStaticAssertDecl(clang::StaticAssertDecl const*) {
        // ignore
    }

    void VisitVarDecl(clang::VarDecl const* decl) {
        auto const& name = info_.rename_sym(decl);
        auto const& type = info_.define_type(var_type(decl));

        if (auto const* init = decl->getInit()) {
            if (auto cce = clang::dyn_cast<clang::CXXConstructExpr>(init)) {
                auto var = u::add_local_var(scope_, type, name);
                construct(scope_, var, false, cce, var);
            } else if (decl->getType()->isReferenceType()) {
                auto init_type = expr_type(init);
                auto init_expr = visit_expr(init);

                u::add_local_var(
                    scope_, type, name,
                    init_type->isReferenceType() ? init_expr : u::make_addr_of(init_expr));
            } else {
                u::add_local_var(scope_, type, name, visit_expr_val(init));
            }
        } else {
            u::add_local_var(scope_, type, name);
        }
    }

    void VisitFunctionDecl(clang::FunctionDecl const* decl) {
        PUSH_CONTEXT(decl);

        auto const name = info_.encode_name(decl);
        auto const linkage = decl->getLinkageAndVisibility().getLinkage();
        auto const is_static = linkage == clang::Linkage::InternalLinkage ||
                               linkage == clang::Linkage::UniqueExternalLinkage ||
                               linkage == clang::Linkage::NoLinkage || decl->isInlined();

        function_builder f(info_, name);
        if (is_static) {
            f.set_static();
        }

        if (has_annotate(decl, "charm_sycl_inline") || decl->isImplicit()) {
            f.set_inline(true);
        } else if (decl->isInlined()) {
            f.set_inline(false);
        }
        f.set_return_type(decl->getReturnType());

        if (auto method = clang::dyn_cast<clang::CXXMethodDecl>(decl);
            method && method->isInstance()) {
            f.add_param(method->getThisType(), info_.nm().this_name());
        }
        for (auto param : decl->parameters()) {
            f.add_param(param->getType(), info_.rename_sym(param));
        }

        if (auto ctor = clang::dyn_cast<clang::CXXConstructorDecl>(decl)) {
            for (auto init : ctor->inits()) {
                auto this_ = this_ref();

                if (init->isBaseInitializer()) {
                    auto const base_type = info_.define_type(
                        ast_.getPointerType(clang::QualType(init->getBaseClass(), 0)));
                    auto const base_this = u::make_cast(base_type, this_);
                    base_this->to_base = true;

                    if (auto const* inherit =
                            clang::dyn_cast<clang::CXXInheritedCtorInitExpr>(init->getInit())) {
                        construct_inherit(f.body(), decl, base_this, true,
                                          inherit->getConstructor(), decl);
                    } else {
                        auto ctor = clang::dyn_cast<clang::CXXConstructExpr>(init->getInit());

                        construct(f.body(), base_this, true, ctor, nullptr);
                    }
                } else {
                    auto ref = xcml::new_member_ref();
                    ref->member = init->getMember()->getNameAsString();
                    ref->value = this_;

                    auto iv = init->getInit();
                    if (clang::dyn_cast<clang::CXXDefaultInitExpr>(init->getInit())) {
                        iv = init->getMember()->getInClassInitializer();
                    }

                    auto asg = xcml::new_assign_expr();
                    asg->lhs = ref;
                    asg->rhs = visit_expr(f.body(), iv);

                    u::push_expr(f.body(), asg);
                }
            }
        }

        if (decl->hasBody()) {
            auto body_node = visit_compound_stmt(decl->getBody(), decl, false);

            if (!is_empty(body_node)) {
                if (f.is_body_defined()) {
                    u::push_stmt(f.body(), body_node);
                } else {
                    f.set_body(body_node);
                }
            }
        }

        if (is_runtime_extern(decl)) {
            assert(!decl->hasBody());
            f.declare_runtime_extern(
                std::string_view(decl->getName().data(), decl->getName().size()));
        }
    }

private:
    clang::QualType var_type(clang::VarDecl const* decl) {
        return var_type(decl->getType());
    }

    clang::QualType var_type(clang::QualType type) {
        if (!type->isReferenceType() && !type->isPointerType() && type.isConstQualified()) {
            type.removeLocalConst();
        }
        return type;
    }
};

void visitor_base::visit_decl(xcml::compound_stmt_ptr const& scope, clang::Decl const* decl,
                              context const&) {
    decl_visitor vis(info_, scope, current_kernel_, current_func_, current_is_kernel_);
    return vis.Visit(decl);
}
