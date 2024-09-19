#pragma once

#include <string>

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

#include <iostream>
#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <llvm/Support/raw_ostream.h>

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

void Transform(llvm::StringRef name, clang::ASTContext& ctx, clang::Expr const* range,
               clang::Expr const* offset, clang::Expr const* lambda, std::ostream&);
void TransformSave(llvm::raw_ostream& out);
