#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

struct context_impl final : runtime::context {
    context_impl(device_ptr const& dev);

    context_impl(vec_view<device_ptr> const& devList);

    platform_ptr get_platform() override;

    vec<device_ptr> const& get_devices() override;

private:
    platform_ptr check_platform();

    vec<device_ptr> devs_;
    platform_ptr platform_;
};

context_impl::context_impl(device_ptr const& dev) : devs_({dev}), platform_(check_platform()) {}

context_impl::context_impl(vec_view<device_ptr> const& devList)
    : devs_(devList.data(), devList.size()), platform_(check_platform()) {}

platform_ptr context_impl::get_platform() {
    return platform_;
}

vec<device_ptr> const& context_impl::get_devices() {
    return devs_;
}

platform_ptr context_impl::check_platform() {
    if (devs_.empty()) {
        throw std::runtime_error("No devices found");
    }

    return devs_[0]->get_platform();
}

}  // namespace runtime::impl

namespace runtime {

intrusive_ptr<context> make_context(device_ptr const& dev) {
    return make_intrusive<impl::context_impl>(dev);
}

intrusive_ptr<context> make_context(vec_view<intrusive_ptr<device>> const& devList) {
    return make_intrusive<impl::context_impl>(devList);
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
