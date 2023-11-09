#include <xcml_type.hpp>
#include "transform_info.hpp"

struct struct_builder::impl {
    explicit impl(transform_info& info, std::string_view type, std::string_view name)
        : info_(info), st_(xcml::new_struct_type()), id_(xcml::new_symbol_id()) {
        st_->type = type;
        id_->name = name;
        id_->sclass = xcml::storage_class::tagname;
        id_->type = type;
    }

    transform_info& info_;
    xcml::struct_type_ptr st_;
    xcml::symbol_id_ptr id_;
};

struct_builder::struct_builder(transform_info& info, std::string_view type,
                               std::string_view name)
    : pimpl_(std::make_unique<impl>(info, type, name)) {}

struct_builder::~struct_builder() {
    finalize();
}

struct_builder& struct_builder::add(std::string_view name, clang::QualType const& qty) {
    auto id = xcml::new_symbol_id();
    id->name = name;
    id->type = pimpl_->info_.define_type(qty);
    pimpl_->st_->symbols.push_back(id);
    return *this;
}

void struct_builder::finalize() {
    auto prg = pimpl_->info_.prg();

    prg->type_table.push_back(pimpl_->st_);
    prg->global_symbols.push_back(pimpl_->id_);
}
