#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct platform {
    virtual ~platform() = default;

    virtual ::sycl::backend get_backend() const = 0;

    virtual std::vector<std::shared_ptr<device>> get_devices() = 0;

    virtual std::string info_version() const = 0;

    virtual std::string info_name() const = 0;

    virtual std::string info_vendor() const = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
