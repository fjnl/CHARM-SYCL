#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <int _Dimensions>
struct is_group<group<_Dimensions>> : std::true_type {};

template <class Group>
void group_barrier(Group const& g, memory_scope fence_scope) {
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

namespace runtime {

#ifndef __SYCL_DEVICE_ONLY__
CHARM_SYCL_HOST_INLINE void __charm_sycl_group_barrier(void const*, memory_scope) {}
#endif

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
