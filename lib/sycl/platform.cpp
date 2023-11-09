#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

platform_impl::platform_impl(std::shared_ptr<dep::platform> p) : p_(p) {}

sycl::backend platform_impl::get_backend() const {
    return sycl::backend::charm;
}

std::vector<std::shared_ptr<runtime::device>> platform_impl::get_devices() {
    std::vector<std::shared_ptr<runtime::device>> ret;

    for (auto const& d : p_->get_devices()) {
        ret.push_back(impl::make_device(shared_from_this(), d));
    }

    return ret;
}

std::string platform_impl::info_version() const {
    return p_->info_version();
}

std::string platform_impl::info_name() const {
    return p_->info_name();
}

std::string platform_impl::info_vendor() const {
    return p_->info_vendor();
}

std::shared_ptr<platform_impl> make_platform(std::shared_ptr<dep::platform> p) {
    return std::make_shared<platform_impl>(p);
}

}  // namespace runtime::impl

namespace runtime {

std::vector<std::shared_ptr<platform>> get_platforms() {
    std::vector<std::shared_ptr<platform>> ret;

    for (auto p : impl::global_state::get_depmgr()->get_platforms()) {
        ret.push_back(impl::make_platform(p));
    }

    return ret;
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
