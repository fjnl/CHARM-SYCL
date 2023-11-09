#include <clang-sycl.hpp>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include "transform.hpp"

namespace {

static llvm::cl::OptionCategory options("CHARM Options");

static llvm::cl::opt<std::string> optOutput("output", llvm::cl::desc("Output file name"),
                                            llvm::cl::value_desc("FILE"), llvm::cl::init("-"),
                                            llvm::cl::cat(options));

struct OptionsParser : clang::tooling::CommonOptionsParser {
    OptionsParser(int& argc, const char** argv) : CommonOptionsParser(argc, argv, options) {}
};

}  // namespace

int main(int argc, const char** argv) {
    auto op = clang::tooling::CommonOptionsParser::create(argc, argv, options);
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

    auto action = clsy::ast_consume_frontend_action([&](clang::ASTContext& ctx) {
        clsy::visit_kernel_functions(
            ctx, [&](llvm::StringRef name, clang::CXXMemberCallExpr* call, clang::Expr* range,
                     clang::Expr const* offset, clang::Expr* fn) {
                Transform(name, ctx, range, offset, fn);
            });
    });

    auto status = clsy::run_action(op.get(), std::move(action));

    if (status == 0) {
        TransformSave(os ? *os : llvm::outs());
    }
    return status;
}
