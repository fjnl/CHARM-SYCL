#include "visitor_impl.hpp"

struct decl_visitor : decl_visitor_base<decl_visitor, void> {
    using decl_visitor_base::decl_visitor_base;

    void VisitStaticAssertDecl(clang::StaticAssertDecl const*) {
        // ignore
    }

    void VisitTypedefDecl(clang::TypedefDecl const*) {
        // ignore
    }

    void VisitVarDecl(clang::VarDecl const* decl) {
        PUSH_CONTEXT(decl);

        auto const& name = info_.rename_sym(decl);
        auto const& type = info_.define_type(var_type(decl));

        if (auto const* init = decl->getInit()) {
            if (auto const* list = clang::dyn_cast<clang::InitListExpr>(init)) {
                if (!decl->getType()->isArrayType()) {
                    not_supported(ast_, "not an array");
                }
                if (!decl->getType()->isConstantArrayType()) {
                    not_supported(ast_, "not a constant array");
                }

                auto const array = u::add_local_var(scope_, type, name);

                auto const len = clang::dyn_cast<clang::ConstantArrayType>(decl->getType())
                                     ->getSize()
                                     .getZExtValue();

                for (uint64_t i = 0; i < len; i++) {
                    auto aref = xcml::new_array_ref();
                    aref->array = array;
                    aref->index.push_back(xcml::utils::lit(i));

                    auto asg = xcml::new_assign_expr();
                    asg->lhs = aref;

                    if (i < list->getNumInits()) {
                        asg->rhs = visit_expr(list->getInit(i));
                    } else {
                        auto const* filler = list->getArrayFiller();
                        if (!filler || !clang::dyn_cast<clang::ImplicitValueInitExpr>(filler)) {
                            not_supported(ast_, "filler is not supported");
                        }

                        asg->rhs = xcml::utils::lit(0);
                    }

                    push_expr(scope_, list, asg);
                }
            } else if (auto cce = clang::dyn_cast<clang::CXXConstructExpr>(init)) {
                auto var = u::add_local_var(scope_, type, name);
                construct(scope_, var, false, cce, var);
            } else if (decl->getType()->isReferenceType()) {
                auto init_type = expr_type(init);
                auto init_expr = visit_expr(init);

                u::add_local_var(
                    scope_, type, name,
                    init_type->isReferenceType() ? init_expr : u::make_addr_of(init_expr));
            } else {
                u::add_local_var(scope_, type, name, visit_expr(init));
            }
        } else {
            u::add_local_var(scope_, type, name);
        }
    }

    void VisitFunctionDecl(clang::FunctionDecl const* decl) {
        PUSH_CONTEXT(decl);

        if (decl->isDefined()) {
            decl = decl->getDefinition();
        }

        auto const name = info_.encode_name(decl);
        auto const linkage = decl->getLinkageAndVisibility().getLinkage();
        auto const is_static = linkage == clang::Linkage::InternalLinkage ||
                               linkage == clang::Linkage::UniqueExternalLinkage ||
                               linkage == clang::Linkage::NoLinkage || decl->isInlined();
        auto const* method = clang::dyn_cast<clang::CXXMethodDecl>(decl);
        auto const* record = method ? method->getParent() : nullptr;

        function_builder f(info_, name);

        set_loc(f.defi(), decl);

        if (is_static) {
            f.set_static();
        }

        if (has_annotate(decl, "charm_sycl_inline") || decl->isImplicit()) {
            f.set_inline(true);
        } else if (decl->isInlined()) {
            f.set_inline(false);
        }
        f.set_return_type(decl->getReturnType());

        if (method && method->isInstance()) {
            auto type = method->getThisType();

            if (record && info_.is_kernel(record)) {
                auto const is_ptr = type->isPointerType();
                if (type->isPointerType()) {
                    type = type->getPointeeType();
                }

                type.removeLocalConst();

                if (is_ptr) {
                    type = ast_.getPointerType(type);
                }
            }

            f.add_param(type, info_.nm().this_name());
        }
        for (auto param : decl->parameters()) {
            auto type = param->getType();
            f.add_param(type, info_.rename_sym(param));
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
                    asg->rhs = visit_expr_val(f.body(), iv);

                    u::push_expr(f.body(), asg);
                }
            }
        }

        if (record && info_.is_kernel(record)) {
            auto& ctx = info_.ctx();
            auto const fn_record = ctx.getRecordType(record);
            auto const this_ = u::make_var_ref(info_.nm().this_name());

            auto const l = layout(ctx, fn_record, true);
            for (auto it = l.begin(); it != l.end(); ++it) {
                auto const type = it->decl()->getType();

                if (accessor_type acc_type;
                    ::is_accessor(type, acc_type) && acc_type == accessor_type::LOCAL) {
                    static auto const* const BASE_FN = "__charm_sycl_local_memory_base";

                    xcml::expr_ptr acc_ptr = this_;

                    for (auto const* f : it->path()) {
                        acc_ptr = u::make_member_ref(acc_ptr, l.get_field_name(f));

                        if (!f->getType()->isPointerType()) {
                            acc_ptr = u::make_addr_of(acc_ptr);
                        }
                    }

                    bool base_fn_found = false;
                    for (auto const& decl : info_.prg()->global_declarations) {
                        if (auto fd = xcml::function_decl::dyncast(decl);
                            fd && fd->name == BASE_FN) {
                            base_fn_found = true;
                            break;
                        }
                    }

                    if (!base_fn_found) {
                        function_builder b(info_, BASE_FN);
                        b.set_return_type(ctx.VoidPtrTy);
                    }

                    auto ptr_ref = u::make_member_ref(acc_ptr, "ptr");

                    u::push_expr(
                        f.body(),
                        u::assign_expr(ptr_ref, u::make_call(u::make_func_addr(BASE_FN), {})));
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
