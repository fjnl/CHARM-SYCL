#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {
void __charm_sycl_group_barrier(void const*, memory_scope);
}  // namespace runtime

template <int _Dimensions>
struct is_group<group<_Dimensions>> : std::true_type {};

template <class Group>
void group_barrier(Group& g, memory_scope fence_scope) {
#ifdef __SYCL_DEVICE_ONLY__
    if constexpr (std::is_same_v<std::decay_t<Group>, group<1>> ||
                  std::is_same_v<std::decay_t<Group>, group<2>> ||
                  std::is_same_v<std::decay_t<Group>, group<3>>) {
        runtime::__charm_sycl_group_barrier(&g, fence_scope);
    } else {
        static_assert(not_supported<Group>, "not supported group");
    }
#else
    (void)g;
    (void)fence_scope;
#endif
}

template <int Dimensions>
template <class WorkItemFunctionT>
void group<Dimensions>::parallel_for_work_item(WorkItemFunctionT const& func) const {
    /* scope: wi entry */

    if constexpr (Dimensions == 1) {
        sycl::range<1> local_range(runtime::__charm_sycl_local_range3());
        sycl::id<1> local_id(runtime::__charm_sycl_local_id3());

        func(set_local(local_range, local_id).into_h_item());
    } else if constexpr (Dimensions == 2) {
        sycl::range<2> local_range(runtime::__charm_sycl_local_range3(),
                                   runtime::__charm_sycl_local_range2());
        sycl::id<2> local_id(runtime::__charm_sycl_local_id3(),
                             runtime::__charm_sycl_local_id2());

        func(set_local(local_range, local_id).into_h_item());
    } else {
        sycl::range<3> local_range(runtime::__charm_sycl_local_range3(),
                                   runtime::__charm_sycl_local_range2(),
                                   runtime::__charm_sycl_local_range1());
        sycl::id<3> local_id(runtime::__charm_sycl_local_id3(),
                             runtime::__charm_sycl_local_id2(),
                             runtime::__charm_sycl_local_id1());

        func(set_local(local_range, local_id).into_h_item());
    }
}

template <int Dimensions>
h_item<Dimensions> group<Dimensions>::into_h_item() const {
    return h_item<Dimensions>(*this);
}

template <int Dimensions>
nd_item<Dimensions> group<Dimensions>::into_nd_item() const {
    return nd_item<Dimensions>(*this);
}

template <int Dimensions>
item<Dimensions> h_item<Dimensions>::get_global() const {
    return detail::make_item(g_.get_group_range() * get_local_range(),
                             g_.get_group_id() * g_.get_local_range() + g_.get_local_id());
}

template <int Dimensions>
item<Dimensions> h_item<Dimensions>::get_local() const {
    return get_logical_local();
}

template <int Dimensions>
item<Dimensions> h_item<Dimensions>::get_logical_local() const {
    // TODO:
    return get_physical_local();
}

template <int Dimensions>
item<Dimensions> h_item<Dimensions>::get_physical_local() const {
    return g_.get_local_id();
}

template <int Dimensions>
range<Dimensions> h_item<Dimensions>::get_global_range() const {
    return get_global().get_range();
}

template <int Dimensions>
size_t h_item<Dimensions>::get_global_range(int dimension) const {
    return get_global().get_range(dimension);
}

template <int Dimensions>
id<Dimensions> h_item<Dimensions>::get_global_id() const {
    return get_global().get_id();
}

template <int Dimensions>
size_t h_item<Dimensions>::get_global_id(int dimension) const {
    return get_global().get_id(dimension);
}

template <int Dimensions>
range<Dimensions> h_item<Dimensions>::get_local_range() const {
    return get_local().get_range();
}

template <int Dimensions>
size_t h_item<Dimensions>::get_local_range(int dimension) const {
    return get_local().get_range(dimension);
}

template <int Dimensions>
id<Dimensions> h_item<Dimensions>::get_local_id() const {
    return get_local().get_id();
}

template <int Dimensions>
size_t h_item<Dimensions>::get_local_id(int dimension) const {
    return get_local().get_id(dimension);
}

template <int Dimensions>
range<Dimensions> h_item<Dimensions>::get_logical_local_range() const {
    return get_logical_local().get_range();
}

template <int Dimensions>
size_t h_item<Dimensions>::get_logical_local_range(int dimension) const {
    return get_logical_local().get_range(dimension);
}

template <int Dimensions>
id<Dimensions> h_item<Dimensions>::get_logical_local_id() const {
    return get_logical_local().get_id();
}

template <int Dimensions>
size_t h_item<Dimensions>::get_logical_local_id(int dimension) const {
    return get_logical_local().get_id(dimension);
}

template <int Dimensions>
range<Dimensions> h_item<Dimensions>::get_physical_local_range() const {
    return get_physical_local().get_range();
}

template <int Dimensions>
size_t h_item<Dimensions>::get_physical_local_range(int dimension) const {
    return get_physical_local().get_range(dimension);
}

template <int Dimensions>
id<Dimensions> h_item<Dimensions>::get_physical_local_id() const {
    return get_physical_local().get_id();
}

template <int Dimensions>
size_t h_item<Dimensions>::get_physical_local_id(int dimension) const {
    return get_physical_local().get_id(dimension);
}

template <int Dimensions>
range<Dimensions> h_item<Dimensions>::get_physical_group_range() {
    if constexpr (Dimensions == 1) {
        return range<Dimensions>(runtime::__charm_sycl_group_range3());
    } else if constexpr (Dimensions == 2) {
        return range<Dimensions>(runtime::__charm_sycl_group_range3(),
                                 runtime::__charm_sycl_group_range2());
    } else {
        return range<Dimensions>(runtime::__charm_sycl_group_range3(),
                                 runtime::__charm_sycl_group_range2(),
                                 runtime::__charm_sycl_group_range1());
    }
}

template <int Dimensions>
id<Dimensions> h_item<Dimensions>::get_physical_group_id() {
    if constexpr (Dimensions == 1) {
        return id<Dimensions>(runtime::__charm_sycl_group_id3());
    } else if constexpr (Dimensions == 2) {
        return id<Dimensions>(runtime::__charm_sycl_group_id3(),
                              runtime::__charm_sycl_group_id2());
    } else {
        return id<Dimensions>(runtime::__charm_sycl_group_id3(),
                              runtime::__charm_sycl_group_id2(),
                              runtime::__charm_sycl_group_id1());
    }
}

template <int Dimensions>
nd_item<Dimensions> h_item<Dimensions>::into_nd_item() const {
    return g_.into_nd_item();
}

CHARM_SYCL_END_NAMESPACE
