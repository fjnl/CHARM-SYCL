#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace info {

enum class device_type : uint32_t {
    cpu,
    gpu,
    accelerator,
    custom,
    automatic,
    host,
    fpga,
    all,
};

}

CHARM_SYCL_END_NAMESPACE
