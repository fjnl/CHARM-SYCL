#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

/// @see 4.6.4.3. Device aspects
enum class aspect : uint32_t {
    cpu = 1u << 0,
    gpu = 1u << 1,
    accelerator = 1u << 2,
    custom = 1u << 3,
    emulated = 1u << 4,
    host_debuggable = 1u << 5,
    fp16 = 1u << 6,
    fp64 = 1u << 7,
    atomic64 = 1u << 8,
    image = 1u << 9,
    online_compiler = 1u << 10,
    online_linker = 1u << 11,
    queue_profiling = 1u << 12,
    usm_device_allocations = 1u << 13,
    usm_host_allocations = 1u << 14,
    usm_atomic_host_allocations = 1u << 15,
    usm_shared_allocations = 1u << 16,
    usm_atomic_shared_allocations = 1u << 17,
    usm_system_allocations = 1u << 18,
};

CHARM_SYCL_END_NAMESPACE
