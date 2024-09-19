#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <typename DataT, int Dimensions>
template <int, class>
local_accessor<DataT, Dimensions>::local_accessor(handler& h, property_list const&)
#ifndef __SYCL_DEVICE_ONLY__
    : impl_(runtime::make_local_accessor(runtime::impl_access::get_impl(h), Dimensions,
                                         sizeof(DataT), alignof(DataT), range<3>(1, 1, 1)))
#endif
{
}

template <typename DataT, int Dimensions>
template <int, class>
local_accessor<DataT, Dimensions>::local_accessor(range<Dimensions> const& size, handler& h,
                                                  property_list const&)
#ifndef __SYCL_DEVICE_ONLY__
    : impl_(runtime::make_local_accessor(runtime::impl_access::get_impl(h), Dimensions,
                                         sizeof(DataT), alignof(DataT), detail::extend(size)))
#endif
{
}

template <typename DataT, int Dimensions>
typename local_accessor<DataT, Dimensions>::size_type local_accessor<DataT, Dimensions>::size()
    const noexcept {
#ifdef __SYCL_DEVICE_ONLY__
    if constexpr (Dimensions == 1) {
        return size0;
    } else if constexpr (Dimensions == 2) {
        return size0 * size1;
    } else {
        return size0 * size1 * size2;
    }
#else
    return impl_->size();
#endif
}

template <typename DataT, int Dimensions>
range<Dimensions> local_accessor<DataT, Dimensions>::get_range() const {
#ifdef __SYCL_DEVICE_ONLY__
    if constexpr (Dimensions == 1) {
        return {size0};
    } else if constexpr (Dimensions == 2) {
        return {size0, size1};
    } else {
        return {size0, size1, size2};
    }
#else
    auto const& r = impl_->get_range();

    if constexpr (Dimensions == 1) {
        return {r[0]};
    } else if constexpr (Dimensions == 2) {
        return {r[0], r[1]};
    } else {
        return {r[0], r[1], r[2]};
    }
#endif
}

template <typename DataT, int Dimensions>
typename local_accessor<DataT, Dimensions>::pointer_type
local_accessor<DataT, Dimensions>::get_pointer() const noexcept {
#ifdef __SYCL_DEVICE_ONLY__
    if constexpr (Dimensions >= 1) {
        runtime::__charm_sycl_assume(off % 16 == 0);
        return reinterpret_cast<pointer_type>(reinterpret_cast<std::byte*>(ptr) + off);
    } else {
        return reinterpret_cast<pointer_type>(reinterpret_cast<std::byte*>(ptr) + off);
    }
#else
    return reinterpret_cast<pointer_type>(impl_->get_pointer());
#endif
}

CHARM_SYCL_END_NAMESPACE
