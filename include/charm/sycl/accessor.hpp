#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

template <class DataT, int Dimensions, access_mode AccessMode, target Target>
using accessor_impl = std::conditional_t<
    Target == target::host_buffer, host_accessor<DataT, Dimensions, AccessMode>,
    std::conditional_t<Target == target::local, local_accessor<DataT, Dimensions>,
                       device_accessor<DataT, Dimensions, AccessMode>>>;

}

template <class DataT, int Dimensions, access_mode AccessMode, target Target>
struct accessor : detail::accessor_impl<DataT, Dimensions, AccessMode, Target> {
private:
    using impl_t = detail::accessor_impl<DataT, Dimensions, AccessMode, Target>;

public:
    using impl_t::impl_t;
};

template <class DataT, int Dimensions, class AllocatorT, class TagT,
          class = std::enable_if_t<detail::is_tag_v<TagT>>>
accessor(buffer<DataT, Dimensions, AllocatorT>&, handler&, TagT)
    -> accessor<DataT, Dimensions, detail::to_mode_v<TagT>, target::device>;

template <class DataT, int Dimensions, class AllocatorT, class TagT,
          class = std::enable_if_t<detail::is_tag_v<TagT>>>
accessor(buffer<DataT, Dimensions, AllocatorT>&, handler&, TagT, property_list const&)
    -> accessor<DataT, Dimensions, detail::to_mode_v<TagT>, target::device>;

template <class DataT, int Dimensions, class AllocatorT, class TagT,
          class = std::enable_if_t<detail::is_tag_v<TagT>>>
accessor(buffer<DataT, Dimensions, AllocatorT>&, TagT)
    -> accessor<DataT, Dimensions, detail::to_mode_v<TagT>, target::host_buffer>;

template <class DataT, int Dimensions, class AllocatorT, class TagT,
          class = std::enable_if_t<detail::is_tag_v<TagT>>>
accessor(buffer<DataT, Dimensions, AllocatorT>&, TagT, property_list const&)
    -> accessor<DataT, Dimensions, detail::to_mode_v<TagT>, target::host_buffer>;

CHARM_SYCL_END_NAMESPACE
