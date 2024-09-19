#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

platform_impl::platform_impl(std::shared_ptr<dep::platform> p) : p_(p) {}

sycl::backend platform_impl::get_backend() const {
    return sycl::backend::charm;
}

vec<intrusive_ptr<device>> platform_impl::get_devices() {
    vec<intrusive_ptr<device>> ret;

    for (auto const& d : p_->get_devices()) {
        ret.push_back(impl::make_device(intrusive_from_this(), d));
    }

    return ret;
}

vec<char> platform_impl::info_version() const {
    return p_->info_version();
}

vec<char> platform_impl::info_name() const {
    return p_->info_name();
}

vec<char> platform_impl::info_vendor() const {
    return p_->info_vendor();
}

intrusive_ptr<platform_impl> make_platform(std::shared_ptr<dep::platform> p) {
    return make_intrusive<platform_impl>(p);
}

}  // namespace runtime::impl

namespace runtime {

vec<intrusive_ptr<platform>> get_platforms() {
    vec<intrusive_ptr<platform>> ret;

    for (auto p : impl::global_state::get_depmgr()->get_platforms()) {
        ret.push_back(impl::make_platform(p));
    }

    return ret;
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
