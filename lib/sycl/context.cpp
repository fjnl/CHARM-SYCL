#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

struct context_impl final : runtime::context {
    context_impl(std::shared_ptr<runtime::device> const& dev, async_handler asyncHandler,
                 property_list const*);

    context_impl(std::vector<std::shared_ptr<runtime::device>> const& devList,
                 async_handler asyncHandler, property_list const*);

    std::shared_ptr<runtime::platform> get_platform() override;

    std::vector<std::shared_ptr<runtime::device>> const& get_devices() override;

    async_handler& get_async_handler() override;

private:
    std::shared_ptr<runtime::platform> check_platform();

    std::vector<std::shared_ptr<runtime::device>> devs_;
    async_handler ah_;
    std::shared_ptr<runtime::platform> platform_;
};

context_impl::context_impl(std::shared_ptr<runtime::device> const& dev,
                           async_handler asyncHandler, property_list const*)
    : devs_({dev}), ah_(asyncHandler), platform_(check_platform()) {}

context_impl::context_impl(std::vector<std::shared_ptr<runtime::device>> const& devList,
                           async_handler asyncHandler, property_list const*)
    : devs_(devList), ah_(asyncHandler), platform_(check_platform()) {}

std::shared_ptr<runtime::platform> context_impl::get_platform() {
    return platform_;
}

std::vector<std::shared_ptr<runtime::device>> const& context_impl::get_devices() {
    return devs_;
}

async_handler& context_impl::get_async_handler() {
    return ah_;
}

std::shared_ptr<runtime::platform> context_impl::check_platform() {
    if (devs_.empty()) {
        throw std::runtime_error("No devices found");
    }

    return devs_.at(0)->get_platform();
}

}  // namespace runtime::impl

namespace runtime {

std::shared_ptr<context> make_context(std::shared_ptr<runtime::device> const& dev,
                                      async_handler asyncHandler,
                                      property_list const* propList) {
    return std::make_shared<impl::context_impl>(dev, asyncHandler, propList);
}

std::shared_ptr<context> make_context(std::vector<std::shared_ptr<device>> const& devList,
                                      async_handler asyncHandler,
                                      property_list const* propList) {
    return std::make_shared<impl::context_impl>(devList, asyncHandler, propList);
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
