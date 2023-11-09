#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

inline queue::queue(const property_list& propList) : queue(default_selector_v, propList) {}

inline queue::queue(const async_handler& asyncHandler, const property_list& propList)
    : queue(default_selector_v, asyncHandler, propList) {}

template <typename DeviceSelector>
inline queue::queue(const DeviceSelector& deviceSelector, const property_list& propList)
    : queue(deviceSelector, async_handler(), propList) {}

template <typename DeviceSelector>
inline queue::queue(const DeviceSelector& deviceSelector, const async_handler& asyncHandler,
                    const property_list& propList)
    : queue(detail::select_device(deviceSelector), asyncHandler, propList) {}

inline queue::queue(const device& syclDevice, const property_list& propList)
    : queue(syclDevice, async_handler(), propList) {}

inline queue::queue(const device& syclDevice, const async_handler& asyncHandler,
                    const property_list& propList)
    : queue(detail::get_default_context(syclDevice.get_platform()), syclDevice, asyncHandler,
            propList) {}

template <typename DeviceSelector>
inline queue::queue(const context& syclContext, const DeviceSelector& deviceSelector,
                    const property_list& propList)
    : queue(syclContext, detail::select_device(deviceSelector), propList) {}

template <typename DeviceSelector>
inline queue::queue(const context& syclContext, const DeviceSelector& deviceSelector,
                    const async_handler& asyncHandler, const property_list& propList)
    : queue(syclContext, detail::select_device(deviceSelector), asyncHandler, propList) {}

inline queue::queue(const context& syclContext, const device& syclDevice,
                    const property_list& propList)
    : queue(syclContext, syclDevice, async_handler(), propList) {}

inline queue::queue(const context& syclContext, const device& syclDevice,
                    const async_handler& asyncHandler, const property_list& propList)
    : impl_(runtime::make_queue(runtime::impl_access::get_impl(syclContext),
                                runtime::impl_access::get_impl(syclDevice), asyncHandler,
                                runtime::impl_access::get_impl(propList).get())) {}

inline backend queue::get_backend() const noexcept {
    return impl_->get_backend();
}

inline context queue::get_context() const {
    return runtime::impl_access::from_impl<context>(impl_->get_context());
}

inline device queue::get_device() const {
    return runtime::impl_access::from_impl<device>(impl_->get_device());
}

template <typename T>
inline event queue::submit(T cgf) {
    handler cgh(*this);
    cgf(cgh);

    auto const ev = cgh.finalize();
    impl_->add(runtime::impl_access::get_impl(ev));

    return ev;
}

inline void queue::wait() {
    impl_->wait();
}

inline void queue::wait_and_throw() {
    // TODO
    wait();
}

inline void queue::throw_asynchronous() {
    // TODO
}

CHARM_SYCL_END_NAMESPACE
