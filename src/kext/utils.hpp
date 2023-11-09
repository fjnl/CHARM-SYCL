#pragma once

#include <utility>
#include <vector>
#include <xcml_type_fwd.hpp>
#include "error_trace.hpp"

namespace clang {
class ASTContext;
class Expr;
class FieldDecl;
class QualType;
class ValueDecl;
}  // namespace clang

struct transform_info;

std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> captured_vars(
    transform_info& info, clang::Expr const* fn, context const& = context());

bool is_const_lvref(clang::QualType const& type);

void remove_const_lvref(clang::QualType& type);

bool is_empty(xcml::compound_stmt_ptr c);
