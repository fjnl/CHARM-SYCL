#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

inline context::context(const property_list& propList)
    : context(detail::select_device(default_selector_v), propList) {}

inline context::context(async_handler asyncHandler, const property_list& propList)
    : context(detail::select_device(default_selector_v), asyncHandler, propList) {}

inline context::context(const device& dev, const property_list& propList)
    : context(dev, async_handler(), propList) {}

inline context::context(const device& dev, async_handler asyncHandler,
                        const property_list& propList)
    : impl_(runtime::make_context(runtime::impl_access::get_impl(dev), asyncHandler,
                                  runtime::impl_access::get_impl(propList).get())) {}

inline context::context(const std::vector<device>& deviceList, const property_list& propList)
    : context(deviceList, async_handler(), propList) {}

inline context::context(const std::vector<device>& deviceList, async_handler asyncHandler,
                        const property_list& propList) {
    std::vector<std::shared_ptr<runtime::device>> devs;

    devs.reserve(deviceList.size());
    for (auto& d : deviceList) {
        devs.push_back(runtime::impl_access::get_impl(d));
    }

    impl_ = runtime::make_context(devs, asyncHandler,
                                  runtime::impl_access::get_impl(propList).get());
}

inline backend context::get_backend() const noexcept {
    return get_platform().get_backend();
}

inline platform context::get_platform() const {
    return runtime::impl_access::from_impl<platform>(impl_->get_platform());
}

inline std::vector<device> context::get_devices() const {
    return runtime::impl_access::from_impl<device>(impl_->get_devices());
}

namespace detail {

inline context get_default_context(platform plt) {
    return context(plt.get_devices());
}

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
