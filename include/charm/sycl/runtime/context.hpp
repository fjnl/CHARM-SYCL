#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct context {
    virtual ~context() {}

    virtual std::shared_ptr<platform> get_platform() = 0;

    virtual std::vector<std::shared_ptr<device>> const& get_devices() = 0;

    virtual async_handler& get_async_handler() = 0;
};

std::shared_ptr<context> make_context(std::shared_ptr<device> const& dev,
                                      async_handler asyncHandler, property_list const*);

std::shared_ptr<context> make_context(std::vector<std::shared_ptr<device>> const& devList,
                                      async_handler asyncHandler, property_list const*);

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
