#pragma once

#include <utility>
#include <vector>
#include <xcml_type_fwd.hpp>
#include "error_trace.hpp"

namespace clang {
class ASTContext;
class CXXMethodDecl;
class CXXRecordDecl;
class Expr;
class FieldDecl;
class QualType;
class ValueDecl;
}  // namespace clang

struct transform_info;

std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> captured_vars(
    transform_info& info, clang::Expr const* fn, context const& = context());

bool is_const_lvref(clang::QualType const& type);

clang::QualType remove_const_lvref(clang::QualType type);

clang::QualType remove_cvref(clang::QualType type);

bool is_empty(xcml::compound_stmt_ptr c);

enum accessor_type { NONE = 0, NORMAL, LOCAL, DEVICE };

bool is_accessor(clang::QualType type, accessor_type& acc);

clang::CXXMethodDecl const* get_call_operator(transform_info& info, clang::Expr const* fn);
clang::CXXMethodDecl const* get_call_operator(transform_info& info,
                                              clang::CXXRecordDecl const* record);

struct layout {
    struct field;

    explicit layout(clang::ASTContext& ctx, clang::QualType const&, bool recursive = false);

    ~layout();

    size_t size_of() const;

    size_t align_of() const;

    clang::QualType const& type() const;

    field const* begin() const;

    field const* end() const;

    bool empty() const;

    std::string to_string() const;

    void dump() const;

    std::string get_field_name(clang::FieldDecl const* field) const;

private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};

struct layout::field {
    explicit field(layout&, clang::FieldDecl const* decl, size_t offset);

    explicit field(layout&, std::vector<clang::FieldDecl const*> const& path, size_t offset);

    size_t offset_of() const;

    size_t size_of() const;

    // void set_padding(size_t pad);

    // size_t padding_after() const;

    clang::FieldDecl const* decl() const;

    std::vector<clang::FieldDecl const*> const& path() const;

private:
    layout* l_;
    std::vector<clang::FieldDecl const*> path_;
    size_t offset_;
    // size_t pad_;
    // bool final_;
};

bool point_to_refenrece(transform_info& info, clang::Expr const* expr);
bool point_to_pointer(transform_info& info, clang::Expr const* expr);
bool point_to_ref_or_ptr(transform_info& info, clang::Expr const* expr);
