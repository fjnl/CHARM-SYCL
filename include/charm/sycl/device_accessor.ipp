#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

template <class DataT, int Dimensions, access_mode AccessMode>
template <typename AllocatorT, int, class>
device_accessor<DataT, Dimensions, AccessMode>::device_accessor(
    [[maybe_unused]] buffer<DataT, Dimensions, AllocatorT>& bufferRef,
    [[maybe_unused]] handler& commandGroupHandlerRef,
    [[maybe_unused]] range<Dimensions> accessRange,
    [[maybe_unused]] id<Dimensions> accessOffset,
    [[maybe_unused]] const property_list& propList)
#ifndef __SYCL_DEVICE_ONLY__
    : impl_(runtime::make_accessor(runtime::impl_access::get_impl(commandGroupHandlerRef),
                                   runtime::impl_access::get_impl(bufferRef),
                                   detail::extend(accessRange), detail::extend(accessOffset),
                                   AccessMode, runtime::impl_access::get_impl(propList).get()))
#endif
{
}

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
