#include "ast_visitor.hpp"
#include <fmt/format.h>

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
#include <clang/AST/Mangle.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

namespace {

struct Visitor : clang::RecursiveASTVisitor<Visitor> {
    using callback_type = ::callback_type;

    template <class F>
    explicit Visitor(clang::ASTContext& ctx, F const& f, bool outer)
        : ctx_(ctx), fn_(f), outer_(outer) {}

    bool shouldVisitTemplateInstantiations() const {
        return true;
    }

    std::string GetMethodName(clang::CXXMemberCallExpr const* expr) const {
        return expr->getMethodDecl()->getQualifiedNameAsString();
    }

    bool IsSingleTask(clang::CXXMemberCallExpr* expr) const {
        return GetMethodName(expr) == "sycl::handler::single_task_";
    }

    bool IsParallelFor(clang::CXXMemberCallExpr* expr) const {
        if (outer_) {
            return GetMethodName(expr) == "sycl::handler::parallel_for";
        }

        return GetMethodName(expr) == "sycl::handler::parallel_for_";
    }

    clang::Expr* Skip(clang::Expr* expr) const {
        while (expr) {
            if (auto mte = clang::dyn_cast<clang::MaterializeTemporaryExpr>(expr)) {
                expr = mte->getSubExpr();
            } else if (auto ice = clang::dyn_cast<clang::ImplicitCastExpr>(expr)) {
                expr = ice->getSubExpr();
            } else if (auto bte = clang::dyn_cast<clang::CXXBindTemporaryExpr>(expr)) {
                expr = bte->getSubExpr();
            } else if (auto fce = clang::dyn_cast<clang::CXXFunctionalCastExpr>(expr);
                       fce && fce->getCastKind() == clang::CastKind::CK_ConstructorConversion) {
                expr = fce->getSubExpr();
            } else {
                break;
            }
        }
        return expr;
    }

    clang::LambdaExpr* AsLambda(clang::Expr* expr) const {
        return clang::dyn_cast<clang::LambdaExpr>(Skip(expr));
    }

    bool VisitCXXMemberCallExpr(clang::CXXMemberCallExpr* expr) {
        if (!expr->getMethodDecl()) {
            return true;
        }

        auto const single = IsSingleTask(expr);
        auto const parallel = IsParallelFor(expr);

        if (single || parallel) {
            clang::Expr* range = nullptr;
            clang::Expr* offset = nullptr;
            clang::Expr* fn = nullptr;

            auto const* targs = clang::dyn_cast<clang::FunctionDecl>(expr->getCalleeDecl())
                                    ->getTemplateSpecializationArgs();
            auto const kernel_name = targs->get(0).getAsType();

            if (single) {
                // tempalte <class KernelName, class KernelType>
                // event single_task(KenrelType const& kf);
                fn = Skip(expr->getArg(0));
            } else {
                range = expr->getArg(0);

                if (expr->getNumArgs() == 2) {
                    fn = Skip(expr->getArg(1));
                } else {
                    offset = expr->getArg(1);
                    fn = Skip(expr->getArg(2));
                }
            }

            if (auto* lambda = clang::dyn_cast<clang::LambdaExpr>(fn)) {
                exec(expr, range, offset, kernel_name, lambda);
            } else if (auto* dr = clang::dyn_cast<clang::DeclRefExpr>(fn)) {
                exec(expr, range, offset, kernel_name, dr);
            } else if (auto* ctor = clang::dyn_cast<clang::CXXConstructExpr>(fn)) {
                exec(expr, range, offset, kernel_name, ctor);
            } else {
                fmt::print(stderr, "{}:{}:\nNot supported:\n", __FILE__, __LINE__);
                expr->dumpColor();
                exit(1);
            }
        }

        return true;
    }

private:
    void exec(clang::CXXMemberCallExpr* expr, clang::Expr* range, clang::Expr* offset,
              clang::QualType fn_type, clang::Expr* fn) {
        fn_type = fn_type.getNonReferenceType();
        fn_type.removeLocalConst();
        fn_type.removeLocalVolatile();
        auto const name = clang::SYCLUniqueStableNameExpr::ComputeName(ctx_, fn_type);
        fn_(name, expr, range, offset, fn);
    }

    clang::ASTContext& ctx_;
    callback_type fn_;
    bool outer_;
};

struct ASTConsumer : clang::ASTConsumer {
    explicit ASTConsumer(std::function<void(clang::ASTContext&)> f) : f_(f) {}

    virtual void HandleTranslationUnit(clang::ASTContext& ctx) override {
        f_(ctx);
    }

private:
    std::function<void(clang::ASTContext&)> f_;
};

struct FrontendAction : clang::SyntaxOnlyAction {
    explicit FrontendAction(std::function<void(clang::ASTContext&)> const& f) : f_(f) {}

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&,
                                                                  llvm::StringRef) override {
        return std::make_unique<ASTConsumer>(f_);
    }

private:
    std::function<void(clang::ASTContext&)> f_;
};

struct FrontendActionFactory : clang::tooling::FrontendActionFactory {
    explicit FrontendActionFactory(std::function<void(clang::ASTContext&)> const& f) : f_(f) {}

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::unique_ptr<clang::FrontendAction>(new FrontendAction(f_));
    }

private:
    std::function<void(clang::ASTContext&)> f_;
};

}  // namespace

std::unique_ptr<clang::tooling::FrontendActionFactory> ast_consume_frontend_action(
    std::function<void(clang::ASTContext&)> const& f) {
    return std::make_unique<FrontendActionFactory>(f);
}

int run_action(clang::tooling::CommonOptionsParser& op,
               std::unique_ptr<clang::tooling::FrontendActionFactory>&& factory) {
    clang::tooling::ClangTool tool(op.getCompilations(), op.getSourcePathList());

    tool.appendArgumentsAdjuster(clang::tooling::getClangSyntaxOnlyAdjuster());
    tool.appendArgumentsAdjuster(clang::tooling::getClangStripOutputAdjuster());
    tool.appendArgumentsAdjuster(clang::tooling::getClangStripDependencyFileAdjuster());

    return tool.run(factory.get());
}

void visit_kernel_functions(clang::ASTContext& ast, callback_type const& f, bool outer) {
    Visitor(ast, f, outer).TraverseAST(ast);
}
