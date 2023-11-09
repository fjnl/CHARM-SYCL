#pragma once

#include <stdlib.h>
#include <charm/sycl/fwd.hpp>
#include <charm/sycl/id.hpp>
#include <charm/sycl/range.hpp>

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
        if constexpr (Dimensions == 1) {
            return {group_id_[0]};
        } else if constexpr (Dimensions == 2) {
            return {group_id_[0], group_id_[1]};
        } else {
            return {group_id_[0], group_id_[1], group_id_[2]};
        }
    }

    CHARM_SYCL_INLINE size_t get_group_id(int dimension) const {
        return group_id_[dimension];
    }

    CHARM_SYCL_INLINE id<Dimensions> get_local_id() const {
        if constexpr (Dimensions == 1) {
            return {local_id_[0]};
        } else if constexpr (Dimensions == 2) {
            return {local_id_[0], local_id_[1]};
        } else {
            return {local_id_[0], local_id_[1], local_id_[2]};
        }
    }

    CHARM_SYCL_INLINE size_t get_local_id(int dimension) const {
        return local_id_[dimension];
    }

    CHARM_SYCL_INLINE range<Dimensions> get_local_range() const {
        if constexpr (Dimensions == 1) {
            return {local_range_[0]};
        } else if constexpr (Dimensions == 2) {
            return {local_range_[0], local_range_[1]};
        } else {
            return {local_range_[0], local_range_[1], local_range_[2]};
        }
    }

    CHARM_SYCL_INLINE size_t get_local_range(int dimension) const {
        return local_range_[dimension];
    }

    CHARM_SYCL_INLINE range<Dimensions> get_group_range() const {
        if constexpr (Dimensions == 1) {
            return {group_range_[0]};
        } else if constexpr (Dimensions == 2) {
            return {group_range_[0], group_range_[1]};
        } else {
            return {group_range_[0], group_range_[1], local_id_[2]};
        }
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

    // template <typename WorkItemFunctionT>
    // void parallel_for_work_item(const WorkItemFunctionT &func) const;

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
    template <int D>
    friend nd_item<D> detail::make_nd_item();

    template <int D = Dimensions>
    explicit group(std::enable_if_t<D == 1, range<Dimensions>> group_range,
                   range<Dimensions> local_range, id<Dimensions> group_id,
                   id<Dimensions> local_id)
        : group_range_{group_range[0]},
          local_range_{local_range[0]},
          group_id_{group_id[0]},
          local_id_{local_id[0]} {}

    template <int D = Dimensions>
    explicit group(std::enable_if_t<D == 2, range<Dimensions>> group_range,
                   range<Dimensions> local_range, id<Dimensions> group_id,
                   id<Dimensions> local_id)
        : group_range_{group_range[0], group_range[1]},
          local_range_{local_range[0], local_range[1]},
          group_id_{group_id[0], group_id[1]},
          local_id_{local_id[0], local_id[1]} {}

    template <int D = Dimensions>
    explicit group(std::enable_if_t<D == 3, range<Dimensions>> group_range,
                   range<Dimensions> local_range, id<Dimensions> group_id,
                   id<Dimensions> local_id)
        : group_range_{group_range[0], group_range[1], group_range[2]},
          local_range_{local_range[0], local_range[1], local_range[2]},
          group_id_{group_id[0], group_id[1], group_id[2]},
          local_id_{local_id[0], local_id[1], local_id[2]} {}

    size_t group_id_[Dimensions];
    size_t local_id_[Dimensions];
    size_t group_range_[Dimensions];
    size_t local_range_[Dimensions];
    // id<Dimensions> group_id_;
    // id<Dimensions> local_id_;
    // range<Dimensions> group_range_;
    // range<Dimensions> local_range_;
};

CHARM_SYCL_END_NAMESPACE
