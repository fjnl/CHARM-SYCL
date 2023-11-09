#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

inline device::device() : device(default_selector_v) {}

template <typename DeviceSelector>
inline device::device(const DeviceSelector& deviceSelector)
    : impl_(detail::select_device(deviceSelector).impl_) {}

inline backend device::get_backend() const noexcept {
    return impl_->get_backend();
}

inline bool device::is_cpu() const {
    return has(aspect::cpu);
}

inline bool device::is_gpu() const {
    return has(aspect::gpu);
}

inline bool device::is_accelerator() const {
    return has(aspect::accelerator);
}

inline platform device::get_platform() const {
    return runtime::impl_access::from_impl<platform>(impl_->get_platform());
}

inline bool device::has(aspect asp) const {
    return detail::test_enum(impl_->get_aspect(), asp);
}

CHARM_SYCL_END_NAMESPACE
