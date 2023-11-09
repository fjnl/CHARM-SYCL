#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

enum class backend {
#define SYCL_BACKEND_CHARM 1
    charm = 1
};

CHARM_SYCL_END_NAMESPACE
