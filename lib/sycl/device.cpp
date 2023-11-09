#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

device_impl::device_impl(std::shared_ptr<runtime::platform> const& plt,
                         std::shared_ptr<dep::device> dev)
    : plt_(plt), dev_(dev) {}

sycl::backend device_impl::get_backend() const {
    return sycl::backend::charm;
}

std::shared_ptr<runtime::platform> const& device_impl::get_platform() {
    return plt_;
}

sycl::aspect device_impl::get_aspect() const {
    return static_cast<sycl::aspect>(0);
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

std::string device_impl::info_name() const {
    return dev_->info_name();
}

std::string device_impl::info_vendor() const {
    return dev_->info_vendor();
}

std::string device_impl::info_driver_version() const {
    return dev_->info_driver_version();
}

std::shared_ptr<dep::device> device_impl::to_lower() {
    return dev_;
}

std::shared_ptr<device_impl> make_device(std::shared_ptr<runtime::platform> const& p,
                                         std::shared_ptr<dep::device> d) {
    return std::make_shared<device_impl>(p, d);
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
