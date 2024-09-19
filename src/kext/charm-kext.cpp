#include <fstream>
#include "ast_visitor.hpp"
#include "transform.hpp"

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

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

#ifdef IMPLEMENT_MAIN
int main(int argc, char** argv)
#else
int kext_main(int argc, char** argv)
#endif
{
    llvm::cl::OptionCategory options("CHARM Options");

    llvm::cl::opt<std::string> optOutput("output", llvm::cl::desc("Output file name"),
                                         llvm::cl::value_desc("FILE"), llvm::cl::init("-"),
                                         llvm::cl::cat(options));
    llvm::cl::opt<std::string> optDesc("desc", llvm::cl::desc("Kernel descriptors file name"),
                                       llvm::cl::value_desc("FILE"), llvm::cl::init("-"),
                                       llvm::cl::cat(options));

    auto op = clang::tooling::CommonOptionsParser::create(argc, const_cast<const char**>(argv),
                                                          options);
    if (auto err = op.takeError()) {
        llvm::errs() << err << "\n";
        return 1;
    }

    std::unique_ptr<llvm::raw_fd_stream> os;
    if (optOutput != "-") {
        std::error_code error;
        os.reset(new llvm::raw_fd_stream(optOutput, error));

        if (error) {
            llvm::errs() << "Error: " << error.message() << "\n";
            return 1;
        }
    }

    std::ofstream desc(optDesc);

    auto action = ast_consume_frontend_action([&](clang::ASTContext& ctx) {
        visit_kernel_functions(
            ctx,
            [&](llvm::StringRef name, clang::CXXMemberCallExpr*, clang::Expr* range,
                clang::Expr const* offset, clang::Expr* fn) {
                Transform(name, ctx, range, offset, fn, desc);
            },
            false);
    });

    auto status = run_action(op.get(), std::move(action));

    if (status == 0) {
        TransformSave(os ? *os : llvm::outs());
    }
    return status;
}
