#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

inline platform::platform() : platform(default_selector_v) {}

template <typename DeviceSelector>
inline platform::platform(const DeviceSelector& deviceSelector) {
    impl_ = detail::select_device(deviceSelector).get_platform().impl_;
}

inline backend platform::get_backend() const noexcept {
    return impl_->get_backend();
}

inline std::vector<device> platform::get_devices(info::device_type type) const {
    std::vector<device> res;

    for (auto dev : impl_->get_devices()) {
        if (type == info::device_type::all || dev->info_device_type() == type) {
            res.push_back(runtime::impl_access::from_impl<device>(dev));
        }
    }

    return res;
}

inline bool platform::has(aspect asp) const {
    auto forall = true;

    for (auto dev : impl_->get_devices()) {
        forall &= detail::test_enum(dev->get_aspect(), asp);
    }

    return forall;
}

inline size_t platform::hash() const {
    return std::hash<decltype(impl_)>()(impl_);
}

inline std::vector<platform> platform::get_platforms() {
    std::vector<platform> res;
    auto const plts = runtime::get_platforms();

    if (plts.empty()) {
        throw exception(make_error_code(errc::runtime), "No SYCL platform is registered");
    }

    for (auto p : plts) {
        res.push_back(runtime::impl_access::from_impl<platform>(p));
    }

    return res;
}

CHARM_SYCL_END_NAMESPACE
