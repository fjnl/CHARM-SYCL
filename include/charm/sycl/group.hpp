#pragma once

#include <stdlib.h>
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <int Dimensions>
struct group {
    using id_type = id<Dimensions>;
    using range_type = range<Dimensions>;
    using linear_id_type = size_t;
    static constexpr int dimensions = Dimensions;
    // static constexpr memory_scope fence_scope = memory_scope::work_group;

    CHARM_SYCL_INLINE inline friend bool operator==(group const& lhs, group const& rhs) {
        return lhs.group_id_ == rhs.group_id_ && lhs.local_id_ == rhs.local_id_ &&
               lhs.group_range_ == rhs.group_range_ && lhs.local_range_ == rhs.local_range_;
    }

    CHARM_SYCL_INLINE inline friend bool operator!=(group const& lhs, group const& rhs) {
        return !(lhs == rhs);
    }

    CHARM_SYCL_INLINE id<Dimensions> get_group_id() const {
        return group_id_;
    }

    CHARM_SYCL_INLINE size_t get_group_id(int dimension) const {
        return group_id_[dimension];
    }

    CHARM_SYCL_INLINE id<Dimensions> get_local_id() const {
        return local_id_;
    }

    CHARM_SYCL_INLINE size_t get_local_id(int dimension) const {
        return local_id_[dimension];
    }

    CHARM_SYCL_INLINE range<Dimensions> get_local_range() const {
        return local_range_;
    }

    CHARM_SYCL_INLINE size_t get_local_range(int dimension) const {
        return local_range_[dimension];
    }

    CHARM_SYCL_INLINE range<Dimensions> get_group_range() const {
        return group_range_;
    }

    CHARM_SYCL_INLINE size_t get_group_range(int dimension) const {
        return group_range_[dimension];
    }

    CHARM_SYCL_INLINE range<Dimensions> get_max_local_range() const {
        return get_local_range();
    }

    CHARM_SYCL_INLINE size_t operator[](int dimension) const {
        return get_group_id(dimension);
    }

    CHARM_SYCL_INLINE size_t get_group_linear_id() const {
        return detail::linear_id<Dimensions>(get_group_range(), get_group_id());
    }

    CHARM_SYCL_INLINE size_t get_local_linear_id() const {
        return detail::linear_id<Dimensions>(get_local_range(), get_local_id());
    }

    CHARM_SYCL_INLINE size_t get_group_linear_range() const {
        return detail::linear_range<Dimensions>(get_group_range());
    }

    CHARM_SYCL_INLINE size_t get_local_linear_range() const {
        return detail::linear_range<Dimensions>(get_local_range());
    }

    CHARM_SYCL_INLINE bool leader() const {
        return get_local_linear_id() == 0;
    }

    template <class WorkItemFunctionT>
    void parallel_for_work_item(WorkItemFunctionT const& func) const;

    // template <typename WorkItemFunctionT>
    // void parallel_for_work_item(range<Dimensions> logicalRange,
    //                             const WorkItemFunctionT &func) const;

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
    friend struct sycl::handler;
    friend struct h_item<Dimensions>;

    template <int D>
    friend nd_item<D> detail::make_nd_item();

    template <int D>
    friend nd_item<D> detail::make_nd_item(range<D> const& group_range,
                                           range<D> const& local_range, id<D> const& group_id,
                                           id<D> const& local_id);
    template <class Group>
    friend void group_barrier(Group& g, memory_scope fence_scope);

    explicit group(range<Dimensions> const& group_range, range<Dimensions> const& local_range,
                   id<Dimensions> const& group_id, id<Dimensions> const& local_id)
        : group_range_(group_range),
          local_range_(local_range),
          group_id_(group_id),
          local_id_(local_id) {}

    group<Dimensions> set_local(range<Dimensions> const& local_range,
                                id<Dimensions> const& local_id) const {
        return group<Dimensions>(group_range_, local_range, group_id_, local_id);
    }

    h_item<Dimensions> into_h_item() const;

    nd_item<Dimensions> into_nd_item() const;

    id<Dimensions> group_id_;
    id<Dimensions> local_id_;
    range<Dimensions> group_range_;
    range<Dimensions> local_range_;
};

template <int Dimensions>
struct h_item {
    static constexpr int dimensions = Dimensions;

    h_item() = delete;

    item<Dimensions> get_global() const;

    item<Dimensions> get_local() const;

    item<Dimensions> get_logical_local() const;

    item<Dimensions> get_physical_local() const;

    range<Dimensions> get_global_range() const;

    size_t get_global_range(int dimension) const;

    id<Dimensions> get_global_id() const;

    size_t get_global_id(int dimension) const;

    range<Dimensions> get_local_range() const;

    size_t get_local_range(int dimension) const;

    id<Dimensions> get_local_id() const;

    size_t get_local_id(int dimension) const;

    range<Dimensions> get_logical_local_range() const;

    size_t get_logical_local_range(int dimension) const;

    id<Dimensions> get_logical_local_id() const;

    size_t get_logical_local_id(int dimension) const;

    range<Dimensions> get_physical_local_range() const;

    size_t get_physical_local_range(int dimension) const;

    id<Dimensions> get_physical_local_id() const;

    size_t get_physical_local_id(int dimension) const;

private:
    friend struct handler;
    friend struct group<Dimensions>;

    explicit h_item(group<Dimensions> const& g) : g_(g) {}

    range<Dimensions> get_physical_group_range();

    id<Dimensions> get_physical_group_id();

    nd_item<Dimensions> into_nd_item() const;

    group<Dimensions> g_;
};

CHARM_SYCL_END_NAMESPACE
