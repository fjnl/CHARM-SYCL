#include <utility>
#include <utils/naming.hpp>
#include <xcml_type.hpp>
#include "transform_info.hpp"

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

#include <clang/AST/ASTContext.h>

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

struct function_builder::impl {
    explicit impl(transform_info& info, std::string_view name) : info_(info), prg_(info.prg()) {
        ft_ = xcml::new_function_type();
        fd_ = xcml::new_function_definition();

        ft_->return_type = "void";
        ft_->type = info_.nm().xfunc_type();
        fd_->name = name;

        sym_ = xcml::new_symbol_id();
        sym_->type = ft_->type;
        sym_->sclass = xcml::storage_class::extern_def;
        sym_->name = fd_->name;

        prg_->type_table.push_back(ft_);
        declare();
        prg_->global_symbols.push_back(sym_);

        info_.add_func(name, ft_, decl_, ref_expr());
    }

    void declare() {
        decl_ = xcml::new_function_decl();
        decl_->name = fd_->name;
        decl_->extern_c = false;

        auto& decls = prg_->global_declarations;
        auto it = decls.begin();
        for (; it != decls.end(); ++it) {
            if (!xcml::function_decl::is_a(*it)) {
                break;
            }
        }

        decls.insert(it, decl_);
    }

    xcml::func_addr_ptr ref_expr() {
        auto x = xcml::new_func_addr();
        x->name = fd_->name;
        return x;
    }

    transform_info& info_;
    bool defined_ = false;
    xcml::xcml_program_node_ptr prg_;
    xcml::function_type_ptr ft_;
    xcml::function_decl_ptr decl_;
    xcml::function_definition_ptr fd_;
    xcml::symbol_id_ptr sym_;
};

function_builder::function_builder(transform_info& info, std::string_view name)
    : pimpl_(std::make_unique<impl>(info, name)) {}

function_builder::~function_builder() = default;

void function_builder::set_static() {
    pimpl_->sym_->sclass = xcml::storage_class::static_;
}

void function_builder::set_extern_c() {
    pimpl_->decl_->extern_c = true;
}

void function_builder::set_return_type(clang::QualType const& type) {
    pimpl_->ft_->return_type = pimpl_->info_.define_type(type);
}

void function_builder::add_attribute(std::string_view attr) {
    auto x = xcml::new_gcc_attribute();
    x->name = attr;
    pimpl_->fd_->attributes.push_back(x);
}

void function_builder::set_inline(bool force) {
    if (force) {
        pimpl_->decl_->inline_ = false;
        pimpl_->decl_->force_inline = true;
    } else {
        pimpl_->decl_->inline_ = true;
        pimpl_->decl_->force_inline = false;
    }
}

void function_builder::declare_runtime_extern(std::string_view kind) {
    auto decl = std::exchange(pimpl_->decl_, {});
    auto& prg = pimpl_->prg_;

    for (auto it = prg->global_declarations.begin(); it != prg->global_declarations.end();
         ++it) {
        if (decl == *it) {
            prg->global_declarations.erase(it);
            break;
        }
    }

    auto rt = xcml::new_runtime_func_decl();
    rt->name = pimpl_->fd_->name;
    rt->func_kind = kind;
    prg->global_declarations.push_back(rt);
}

xcml::var_ref_ptr function_builder::add_param(clang::QualType const& type0,
                                              std::string_view name) {
    auto type(type0);

    if (auto decayed = clang::dyn_cast<clang::DecayedType>(type)) {
        type = decayed->getOriginalType();
    }

    auto param = xcml::new_param_node();
    param->name = name;
    param->type = pimpl_->info_.define_type(type);

    pimpl_->ft_->params.push_back(param);
    pimpl_->fd_->params.push_back(param);

    auto id = xcml::new_symbol_id();
    id->name = name;
    id->type = param->type;
    pimpl_->fd_->symbols.push_back(id);

    auto ref = xcml::new_var_ref();
    ref->name = name;
    return ref;
}

std::string_view function_builder::name() const {
    return pimpl_->fd_->name;
}

void function_builder::set_body(xcml::compound_stmt_ptr body) {
    if (pimpl_->defined_) {
        not_supported(pimpl_->info_.ctx(), "Assertion Failed: already defined");
    }

    if (!pimpl_->defined_) {
        pimpl_->prg_->global_declarations.push_back(pimpl_->fd_);
        pimpl_->defined_ = true;
    }
    pimpl_->fd_->body = body;
}

bool function_builder::is_body_defined() const {
    return pimpl_->defined_;
}

xcml::compound_stmt_ptr function_builder::body() {
    if (!pimpl_->defined_) {
        pimpl_->prg_->global_declarations.push_back(pimpl_->fd_);
        pimpl_->defined_ = true;
    }
    return pimpl_->fd_->body;
}

xcml::func_addr_ptr function_builder::ref_expr() {
    return pimpl_->ref_expr();
}

xcml::function_call_ptr function_builder::call_expr() {
    auto x = xcml::new_function_call();
    x->function = ref_expr();
    return x;
}

std::string const& function_builder::function_type() const {
    return pimpl_->ft_->type;
}

xcml::function_definition_ptr const& function_builder::defi() {
    return pimpl_->fd_;
}
