#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct platform : refcnt_base {
    virtual ~platform() = default;

    virtual ::sycl::backend get_backend() const = 0;

    virtual vec<device_ptr> get_devices() = 0;

    virtual vec<char> info_version() const = 0;

    virtual vec<char> info_name() const = 0;

    virtual vec<char> info_vendor() const = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
