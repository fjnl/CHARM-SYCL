#pragma once

#include <functional>
#include <memory>
#include <clang/AST/AST.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

namespace clsy {

using callback_type = std::function<void(llvm::StringRef, clang::CXXMemberCallExpr*,
                                         clang::Expr*, clang::Expr*, clang::Expr*)>;

void visit_kernel_functions(clang::ASTContext& ast, callback_type const& f);

std::unique_ptr<clang::tooling::FrontendActionFactory> ast_consume_frontend_action(
    std::function<void(clang::ASTContext&)> const& f);

int run_action(clang::tooling::CommonOptionsParser& op,
               std::unique_ptr<clang::tooling::FrontendActionFactory>&& factory);

}  // namespace clsy
