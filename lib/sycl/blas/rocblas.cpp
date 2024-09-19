#include <blas/rocblas_interface.hpp>
#include <blas/rocsolver_interface.hpp>
#include <hip/hip_interface.hpp>
#include "common.hpp"

using BLAS = sycl::runtime::rocblas_interface_40200000;
using SOL = sycl::runtime::rocsolver_interface_40200000;

namespace {

bool initialized = false;
bool lapack_initialized = false;

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace blas {

result<std::string> init_rocblas() {
    if (initialized) {
        return {};
    }

    auto version = CHECK_ERROR(init_rocblas1<BLAS>());

    if (auto result = init_rocblas2<BLAS>(); !result) {
        clear_rocblas1<BLAS>();
        return result;
    }

    if (auto result = init_rocblas3<BLAS>(); !result) {
        clear_rocblas2<BLAS>();
        clear_rocblas1<BLAS>();
        return result;
    }

    initialized = true;

    return version;
}

result<std::string> init_lapack_rocblas() {
    if (lapack_initialized) {
        return {};
    }

    auto version = CHECK_ERROR(init_rocsolver_lapack<SOL>());

    lapack_initialized = true;

    return version;
}

}  // namespace blas

CHARM_SYCL_END_NAMESPACE
