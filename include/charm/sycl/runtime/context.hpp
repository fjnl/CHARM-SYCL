#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct context : refcnt_base {
    virtual ~context() {}

    virtual platform_ptr get_platform() = 0;

    virtual vec<device_ptr> const& get_devices() = 0;
};

context_ptr make_context(device_ptr const& dev);

context_ptr make_context(vec_view<device_ptr> const& devList);

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
