#include <blas/cublas_interface.hpp>
#include <blas/cusolver_interface.hpp>
#include <cuda/cuda_interface.hpp>
#include "common.hpp"

namespace {

bool initialized = false;
bool lapack_initialized = false;

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace blas {

using BLAS = runtime::cublas_interface_11000;
using SOL = runtime::cusolver_interface_11000;

result<std::string> init_cublas() {
    if (initialized) {
        return {};
    }

    auto version = CHECK_ERROR(init_cublas1<BLAS>());

    if (auto result = init_cublas2<BLAS>(); !result) {
        clear_cublas1<BLAS>();
        return result;
    }

    if (auto result = init_cublas3<BLAS>(); !result) {
        clear_cublas2<BLAS>();
        clear_cublas1<BLAS>();
        return result;
    }

    initialized = true;

    return version;
}

result<std::string> init_lapack_cublas() {
    if (lapack_initialized) {
        return {};
    }

    auto version = CHECK_ERROR(init_cusolver_lapack<SOL>());

    lapack_initialized = true;

    return version;
}

}  // namespace blas

CHARM_SYCL_END_NAMESPACE
