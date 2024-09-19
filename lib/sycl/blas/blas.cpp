#include "common.hpp"
#include "logging.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace blas {

void init_blas_cpu() {
    bool blas = false, lapacke = false;
    bool mkl = false, openblas = false, reference = false;

    if (!blas) {
        auto result = init_mkl();

        if (result) {
            INFO("BLAS found: Intel MKL (CPU, indirect)");
            mkl = true;
            blas = true;
        } else {
            INFO(result.error()->description());
        }
    }

    if (!blas) {
        auto result = init_openblas();

        if (result) {
            INFO("BLAS found: OpenBLAS (CPU, indirect)");
            openblas = true;
            blas = true;
        } else {
            INFO(result.error()->description());
        }
    }

    if (!blas) {
        auto result = init_refblas();
        if (result) {
            INFO("BLAS found: Reference BLAS (CPU, indirect)");
            reference = true;
            blas = true;
        } else {
            INFO(result.error()->description());
        }
    }

    if (!blas) {
        WARN("BLAS for CPU is not found");
        return;
    }

    if (!lapacke && mkl) {
        auto result = init_lapack_mkl();
        if (result) {
            INFO("LAPACKE found: Intel MKL (CPU, indirect)");
            lapacke = true;
        } else {
            INFO(result.error()->description());
        }
    }

    if (!lapacke && openblas) {
        auto result = init_lapack_openblas();
        if (result) {
            INFO("LAPACKE found: OpenBLAS (CPU, indirect)");
            lapacke = true;
        } else {
            INFO(result.error()->description());
        }
    }

    if (!lapacke && reference) {
        auto result = init_lapack_refblas();
        if (result) {
            INFO("LAPACKE found: Reference BLAS (CPU, indirect)");
            lapacke = true;
        } else {
            INFO(result.error()->description());
        }
    }

    if (!lapacke) {
        WARN("LAPACKE for CPU is not found");
    }
}

void init_blas_cuda() {
    if (auto result = init_cublas(); result) {
        INFO("BLAS found: cuBLAS {} (CUDA, indirect)", result.value());

        if (auto result = init_lapack_cublas(); result) {
            INFO("LAPACK found: cuSOLVER {} (CUDA, indirect)", result.value());
        } else {
            WARN("LAPACK for CUDA GPUs is not found: {}", result.error()->description());
        }
    } else {
        WARN("BLAS for CUDA GPUs is not found: {}", result.error()->description());
    }
}

void init_blas_hip() {
    if (auto result = init_rocblas(); result) {
        INFO("BLAS found: rocBLAS {} (HIP, indirect)", result.value());

        if (auto result = init_lapack_rocblas(); result) {
            INFO("LAPACK found: rocSOLVER {} (HIP, indirect)", result.value());
        } else {
            WARN("LAPACK for AMD GPUs is not found: {}", result.error()->description());
        }
    } else {
        WARN("BLAS for AMD GPUs is not found: {}", result.error()->description());
    }
}

}  // namespace blas

CHARM_SYCL_END_NAMESPACE
