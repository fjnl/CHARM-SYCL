#pragma once

#include <charm/sycl/config.hpp>
#include <charm/sycl/access_mode.hpp>
#include <charm/sycl/runtime/buffer_impl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

template <class DataT, int Dimensions, access_mode AccessMode>
struct host_accessor_impl {
    host_accessor_impl() {}

    template <class TagT, class AllocatorT>
    explicit host_accessor_impl(buffer<DataT, Dimensions, AllocatorT>& buffer,
                                range<Dimensions> accessRange, id<Dimensions> accessOffset,
                                TagT, property_list const&)
        : range_(accessRange), offset_(accessOffset), buffer_(to_runtime(buffer)) {}

    auto get_range() const {
        return range_;
    }

    auto get_offset() const {
        return offset_;
    }

    auto get_pointer() {
        return buffer_->get_ptr().storage();
    }

private:
    range<Dimensions> range_;
    id<Dimensions> offset_;
    intrusive_ptr<buffer_impl<DataT, Dimensions>> buffer_;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
