#pragma once

#include <functional>
#include <memory>

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

#include <clang/AST/AST.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

using callback_type = std::function<void(llvm::StringRef, clang::CXXMemberCallExpr*,
                                         clang::Expr*, clang::Expr*, clang::Expr*)>;

void visit_kernel_functions(clang::ASTContext& ast, callback_type const& f, bool outer);

std::unique_ptr<clang::tooling::FrontendActionFactory> ast_consume_frontend_action(
    std::function<void(clang::ASTContext&)> const& f);

int run_action(clang::tooling::CommonOptionsParser& op,
               std::unique_ptr<clang::tooling::FrontendActionFactory>&& factory);
