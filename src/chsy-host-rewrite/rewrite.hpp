#pragma once
#include <clang/AST/AST.h>
#include <clang/Rewrite/Core/Rewriter.h>

struct transformer {
    virtual ~transformer() = default;

    virtual void rewrite(clang::Rewriter&, clang::ASTContext&, llvm::StringRef,
                         clang::CXXMemberCallExpr*, clang::Expr*, clang::Expr*) = 0;

    virtual void finalize(clang::Rewriter&) = 0;
};

std::unique_ptr<transformer> make_transformer();
