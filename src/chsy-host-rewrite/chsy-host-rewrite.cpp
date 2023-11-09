#include <clang-sycl.hpp>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <fmt/format.h>
#include <llvm/Support/raw_ostream.h>
#include "rewrite.hpp"

namespace {

static llvm::cl::OptionCategory options("Options");

static llvm::cl::opt<std::string> optOutput("output", llvm::cl::desc("Output file name"),
                                            llvm::cl::value_desc("FILE"), llvm::cl::init("-"),
                                            llvm::cl::cat(options));

}  // namespace

int main(int argc, const char** argv) {
    auto op = clang::tooling::CommonOptionsParser::create(argc, argv, options);
    if (auto err = op.takeError()) {
        llvm::errs() << err << "\n";
        return 1;
    }

    auto action = clsy::ast_consume_frontend_action([](clang::ASTContext& ctx) {
        auto& sm = ctx.getSourceManager();
        clang::Rewriter r(sm, ctx.getLangOpts());
        auto tr = make_transformer();

        clsy::visit_kernel_functions(
            ctx, [&](llvm::StringRef name, clang::CXXMemberCallExpr* call, clang::Expr* range,
                     clang::Expr const* /* offset*/, clang::Expr* fn) {
                tr->rewrite(r, ctx, name, call, range, fn);
            });

        tr->finalize(r);

        auto buffer = r.getRewriteBufferFor(sm.getMainFileID());

        if (!buffer) {
            buffer = &r.getEditBuffer(sm.getMainFileID());
        }

        if (optOutput == "-") {
            buffer->write(llvm::outs());
        } else {
            std::error_code err;
            llvm::raw_fd_ostream out(optOutput, err);

            if (err) {
                llvm::errs() << err.message() << "\n";
                std::exit(1);
            }

            buffer->write(out);
        }
    });

    return clsy::run_action(op.get(), std::move(action));
}
