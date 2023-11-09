#pragma once

#include <xcml_type_fwd.hpp>
#include "error_trace.hpp"

struct transform_info;

namespace clang {
class CompoundStmt;
class Expr;
class FunctionDecl;
}  // namespace clang

xcml::compound_stmt_ptr visit_compound_stmt(
    transform_info& info, clang::CompoundStmt const* stmt, clang::Expr const* kernel,
    clang::FunctionDecl const* function, bool function_is_kernel, context const& = context());

xcml::compound_stmt_ptr visit_compound_stmt(transform_info& info, clang::Stmt const* stmt,
                                            clang::Expr const* kernel,
                                            clang::FunctionDecl const* function,
                                            bool function_is_kernel,
                                            context const& = context());
