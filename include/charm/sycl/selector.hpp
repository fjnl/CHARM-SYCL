#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

struct default_selector {
    int operator()(device const& d) const {
        if (d.is_cpu() || d.is_gpu() || d.is_accelerator()) {
            return 1000;
        }
        return 1;
    }
};

struct cpu_selector {
    int operator()(device const& d) const {
        return d.is_cpu() ? 1 : -1;
    }
};

struct gpu_selector {
    int operator()(device const& d) const {
        return d.is_gpu() ? 1 : -1;
    }
};

struct accelerator_selector {
    int operator()(device const& d) const {
        return d.is_accelerator() ? 1 : -1;
    }
};

}  // namespace detail

struct device_selector {
    virtual ~device_selector() = default;

    virtual int operator()(device const& dev) const = 0;
};

inline constexpr detail::default_selector default_selector_v;
inline constexpr detail::cpu_selector cpu_selector_v;
inline constexpr detail::gpu_selector gpu_selector_v;
inline constexpr detail::accelerator_selector accelerator_selector_v;

using default_selector = decltype(default_selector_v);
using cpu_selector = decltype(cpu_selector_v);
using gpu_selector = decltype(gpu_selector_v);
using accelerator_selector = decltype(accelerator_selector_v);

CHARM_SYCL_END_NAMESPACE
