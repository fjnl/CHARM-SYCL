#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <class DataT, int Dimensions, access_mode AccessMode>
template <typename AllocatorT, int, class>
host_accessor<DataT, Dimensions, AccessMode>::host_accessor(
    buffer<DataT, Dimensions, AllocatorT>& bufferRef, range<Dimensions> accessRange,
    id<Dimensions> accessOffset, const property_list& propList)
    : impl_(runtime::make_host_accessor(runtime::impl_access::get_impl(bufferRef),
                                        detail::extend(accessRange),
                                        detail::extend(accessOffset), AccessMode,
                                        runtime::impl_access::get_impl(propList).get())) {}

CHARM_SYCL_END_NAMESPACE
