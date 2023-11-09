#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct device {
    virtual ~device() = default;

    virtual backend get_backend() const = 0;

    virtual std::shared_ptr<platform> const& get_platform() = 0;

    virtual aspect get_aspect() const = 0;

    virtual sycl::info::device_type info_device_type() const = 0;

    virtual std::string info_name() const = 0;

    virtual std::string info_vendor() const = 0;

    virtual std::string info_driver_version() const = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
