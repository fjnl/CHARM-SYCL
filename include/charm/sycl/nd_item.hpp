#pragma once

#include <stdlib.h>
#include <charm/sycl/group.hpp>
//
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <int Dimensions>
struct nd_item {
    nd_item() = delete;

    CHARM_SYCL_INLINE inline friend bool operator==(nd_item const& lhs, nd_item const& rhs) {
        return lhs.group_ == rhs.group_;
    }

    CHARM_SYCL_INLINE inline friend bool operator!=(nd_item const& lhs, nd_item const& rhs) {
        return !(lhs == rhs);
    }

    CHARM_SYCL_INLINE id<Dimensions> get_global_id() const {
        return compute_global_id(group_.get_group_id(), group_.get_local_range(),
                                 group_.get_local_id());
    }

    CHARM_SYCL_INLINE size_t get_global_id(int dimension) const {
        return get_global_id()[dimension];
    }

    CHARM_SYCL_INLINE size_t get_global_linear_id() const {
        return detail::linear_id<Dimensions>(get_global_range(), get_global_id());
    }

    CHARM_SYCL_INLINE id<Dimensions> get_local_id() const {
        return group_.get_local_id();
    }

    CHARM_SYCL_INLINE size_t get_local_id(int dimension) const {
        return get_local_id()[dimension];
    }

    CHARM_SYCL_INLINE size_t get_local_linear_id() const {
        return detail::linear_id<Dimensions>(get_local_range(), get_local_id());
    }

    CHARM_SYCL_INLINE group<Dimensions> get_group() const {
        return group_;
    }

    // sub_group get_sub_group() const;

    CHARM_SYCL_INLINE size_t get_group(int dimension) const {
        return get_group()[dimension];
    }

    CHARM_SYCL_INLINE size_t get_group_linear_id() const {
        return group_.get_group_linear_id();
    }

    CHARM_SYCL_INLINE range<Dimensions> get_group_range() const {
        return group_.get_group_range();
    }

    CHARM_SYCL_INLINE size_t get_group_range(int dimension) const {
        return get_group_range()[dimension];
    }

    CHARM_SYCL_INLINE range<Dimensions> get_global_range() const {
        if constexpr (Dimensions == 1) {
            return {group_.get_group_range(0) * group_.get_local_range(0)};
        } else if constexpr (Dimensions == 2) {
            return {group_.get_group_range(0) * group_.get_local_range(0),
                    group_.get_group_range(1) * group_.get_local_range(1)};
        } else {
            return {group_.get_group_range(0) * group_.get_local_range(0),
                    group_.get_group_range(1) * group_.get_local_range(1),
                    group_.get_group_range(2) * group_.get_local_range(2)};
        }
    }

    CHARM_SYCL_INLINE size_t get_global_range(int dimension) const {
        return get_global_range()[dimension];
    }

    CHARM_SYCL_INLINE range<Dimensions> get_local_range() const {
        return group_.get_local_range();
    }

    CHARM_SYCL_INLINE size_t get_local_range(int dimension) const {
        return get_local_range()[dimension];
    }

    CHARM_SYCL_INLINE nd_range<Dimensions> get_nd_range() const {
        return nd_range<Dimensions>(get_global_range(), get_local_range());
    }

    void barrier(access::fence_space flag = access::fence_space::global_and_local) const {
        if (flag == access::fence_space::local_space) {
            group_barrier(group_, memory_scope::work_group);
        } else {
            group_barrier(group_, memory_scope::device);
        }
    }

    // template <typename DataT>
    // device_event async_work_group_copy(decorated_local_ptr<DataT> dest,
    //                                    decorated_global_ptr<DataT> src,
    //                                    size_t numElements) const;

    // template <typename DataT>
    // device_event async_work_group_copy(decorated_global_ptr<DataT> dest,
    //                                    decorated_local_ptr<DataT> src,
    //                                    size_t numElements) const;

    // template <typename DataT>
    // device_event async_work_group_copy(decorated_local_ptr<DataT> dest,
    //                                    decorated_global_ptr<DataT> src, size_t numElements,
    //                                    size_t srcStride) const;

    // template <typename DataT>
    // device_event async_work_group_copy(decorated_global_ptr<DataT> dest,
    //                                    decorated_local_ptr<DataT> src, size_t numElements,
    //                                    size_t destStride) const;

    // template <typename... EventTN>
    // void wait_for(EventTN... events) const;

private:
    template <int D>
    friend nd_item<D> detail::make_nd_item();

    friend struct group<Dimensions>;

    explicit nd_item(group<Dimensions> const& g) : group_(g) {}

    static CHARM_SYCL_INLINE inline id<Dimensions> compute_global_id(
        id<Dimensions> const& gid, range<Dimensions> const& range, id<Dimensions> const& id) {
        if constexpr (Dimensions == 1) {
            return {gid[0] * range[0] + id[0]};
        } else if constexpr (Dimensions == 2) {
            return {gid[0] * range[0] + id[0], gid[1] * range[1] + id[1]};
        } else {
            return {gid[0] * range[0] + id[0], gid[1] * range[1] + id[1],
                    gid[2] * range[2] + id[2]};
        }
    }

    group<Dimensions> group_;
};

namespace detail {

template <int D>
sycl::nd_item<D> make_nd_item() {
    if constexpr (D == 1) {
        group<D> g(sycl::range<D>{1}, sycl::range<D>{1}, sycl::id<D>{}, sycl::id<D>{}, nullptr);
        return sycl::nd_item<D>(g);
    } else if constexpr (D == 2) {
        group<D> g(sycl::range<D>{1, 1}, sycl::range<D>{1, 1}, sycl::id<D>{}, sycl::id<D>{},
                   nullptr);
        return sycl::nd_item<D>(g);
    } else {
        group<D> g(sycl::range<D>{1, 1, 1}, sycl::range<D>{1, 1, 1}, sycl::id<D>{},
                   sycl::id<D>{}, nullptr);
        return sycl::nd_item<D>(g);
    }
}

template <int D>
sycl::nd_item<D> make_nd_item(range<D> const& group_range, range<D> const& local_range,
                              id<D> const& group_id, id<D> const& local_id, void* ctx) {
    group<D> g(group_range, local_range, group_id, local_id, ctx);
    return sycl::nd_item<D>(g);
}

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
