#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

device_impl::device_impl(intrusive_ptr<runtime::platform> const& plt,
                         std::shared_ptr<dep::device> dev)
    : plt_(plt), dev_(dev) {}

sycl::backend device_impl::get_backend() const {
    return sycl::backend::charm;
}

intrusive_ptr<runtime::platform> const& device_impl::get_platform() {
    return plt_;
}

sycl::aspect device_impl::get_aspect() const {
    // TODO: correct handling
    return sycl::aspect::fp64;
}

sycl::info::device_type device_impl::info_device_type() const {
    if (dev_->is_cpu()) {
        return sycl::info::device_type::cpu;
    } else if (dev_->is_gpu()) {
        return sycl::info::device_type::gpu;
    } else if (dev_->is_accelerator()) {
        return sycl::info::device_type::accelerator;
    } else if (dev_->is_custom()) {
        return sycl::info::device_type::custom;
    } else if (dev_->is_host()) {
        return sycl::info::device_type::host;
    }
    return static_cast<sycl::info::device_type>(0);
}

vec<char> device_impl::info_name() const {
    return dev_->info_name();
}

vec<char> device_impl::info_vendor() const {
    return dev_->info_vendor();
}

vec<char> device_impl::info_driver_version() const {
    return dev_->info_driver_version();
}

std::shared_ptr<dep::device> device_impl::to_lower() {
    return dev_;
}

intrusive_ptr<device_impl> make_device(intrusive_ptr<runtime::platform> const& p,
                                       std::shared_ptr<dep::device> d) {
    return make_intrusive<device_impl>(p, d);
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
