#pragma once

#include <charm/sycl/config.hpp>
#include "../error.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace blas {

void init_blas_cpu();
void init_blas_cuda();
void init_blas_hip();

}  // namespace blas

CHARM_SYCL_END_NAMESPACE
