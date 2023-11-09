#pragma once

#include <string>
#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <llvm/Support/raw_ostream.h>

void Transform(llvm::StringRef name, clang::ASTContext& ctx, clang::Expr const* range,
               clang::Expr const* offset, clang::Expr const* lambda);
void TransformSave(llvm::raw_ostream& out);
