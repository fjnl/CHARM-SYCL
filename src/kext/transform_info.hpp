#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <xcml_type_fwd.hpp>
#include "error_trace.hpp"

namespace clang {
class ASTContext;
class CXXRecordDecl;
class NamedDecl;
class QualType;
}  // namespace clang

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

    struct func_desc {
        xcml::function_type_ptr type;
        xcml::function_decl_ptr decl;
        xcml::func_addr_ptr addr;
    };

    std::optional<func_desc> find_func(std::string_view name) const;

    bool is_func_defined(std::string_view name) const;

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
