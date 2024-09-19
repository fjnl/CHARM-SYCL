#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <xcml_type_fwd.hpp>
#include "error_trace.hpp"

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
#include <llvm/ADT/DenseMap.h>
#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

namespace clang {
class ASTContext;
class CXXRecordDecl;
class DeclContext;
class FieldDecl;
class ForStmt;
class NamedDecl;
class QualType;
class VarDecl;
class ValueDecl;
}  // namespace clang

#if LLVM_VERSION_MAJOR <= 15
using capture_map_t = llvm::DenseMap<clang::VarDecl const*, clang::FieldDecl*>;
using capture_key_t = clang::VarDecl;
#else
using capture_map_t = llvm::DenseMap<clang::ValueDecl const*, clang::FieldDecl*>;
using capture_key_t = clang::ValueDecl;
#endif

template <class T>
struct scoped_set {
    template <class U>
    explicit scoped_set(T& ref, U&& new_) : ref_(ref), old_(std::forward<U>(new_)) {
        do_swap();
    }

    scoped_set(scoped_set const&) = delete;

    scoped_set(scoped_set&&) = delete;

    scoped_set& operator=(scoped_set const&) = delete;

    scoped_set& operator=(scoped_set&&) = delete;

    ~scoped_set() {
        do_swap();
    }

private:
    void do_swap() {
        using std::swap;
        swap(ref_, old_);
    }

    T& ref_;
    T old_;
};

namespace utils {
struct naming_utils;
}

bool has_annotate(clang::Decl const* decl, std::string_view name);

struct transform_info {
    explicit transform_info(clang::ASTContext& ctx);

    ~transform_info();

    std::string const& define_type(clang::QualType const& type, context const& = context());

    std::string const& rename_sym(clang::NamedDecl const* decl, context const& = context());

    std::string encode_name(clang::Decl const* decl, context const& = context());

    void check_device_copyable(clang::CXXRecordDecl const* decl);

    xcml::xcml_program_node_ptr prg();

    utils::naming_utils& nm();

    clang::ASTContext& ctx();

    clang::ASTContext const& ctx() const;

    void add_type(clang::QualType const& type, std::string_view name);

    void add_func(std::string_view name, xcml::function_type_ptr const& ft,
                  xcml::function_decl_ptr const& decl, xcml::func_addr_ptr const& addr);

    void add_kernel(clang::CXXRecordDecl const* record);

    bool is_kernel(clang::CXXRecordDecl const* record);

    struct func_desc {
        xcml::function_type_ptr type;
        xcml::function_decl_ptr decl;
        xcml::func_addr_ptr addr;
    };

    std::optional<func_desc> find_func(std::string_view name) const;

    bool is_func_defined(std::string_view name) const;

    void add_for_condition(clang::ForStmt const*, xcml::expr_ptr);

    xcml::expr_ptr lookup_for_condition(clang::ForStmt const*);

    scoped_set<std::optional<capture_map_t>> scoped_set_captures(
        clang::CXXRecordDecl const* decl);

    std::optional<capture_map_t> const& current_captures() const;

private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};

struct function_builder {
    explicit function_builder(transform_info& info, std::string_view name);

    ~function_builder();

    void set_static();

    void set_extern_c();

    void set_inline(bool force);

    void set_return_type(clang::QualType const& type);

    void set_body(xcml::compound_stmt_ptr body);

    void add_attribute(std::string_view attr);

    void declare_runtime_extern(std::string_view kind);

    xcml::var_ref_ptr add_param(clang::QualType const& type, std::string_view name);

    std::string_view name() const;

    xcml::compound_stmt_ptr body();

    bool is_body_defined() const;

    xcml::func_addr_ptr ref_expr();

    xcml::function_call_ptr call_expr();

    std::string const& function_type() const;

    xcml::function_definition_ptr const& defi();

private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};

struct struct_builder {
    explicit struct_builder(transform_info& info, std::string_view type, std::string_view name);

    ~struct_builder();

    struct_builder& add(std::string_view name, clang::QualType const& qty);

    void finalize();

private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};
