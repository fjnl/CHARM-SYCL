#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

struct property_list {
    friend struct runtime::impl_access;

    property_list() = default;

    template <class... Properties>
    property_list(Properties&&... props)
        : impl_(runtime::make_property_list(std::forward<Properties>(props)...)) {}

    property_list(property_list&&) = default;

    property_list(property_list const&) = delete;

    property_list& operator=(property_list&&) = default;

    property_list& operator=(property_list const&) = delete;

private:
    std::unique_ptr<runtime::property_list> impl_;
};

CHARM_SYCL_END_NAMESPACE
