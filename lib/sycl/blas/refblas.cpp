#include <blas/cblas_interface_32.hpp>
#include <blas/lapacke_interface_32.hpp>
#include "common.hpp"

namespace {

void* g_handle = nullptr;
void* g_lapacke = nullptr;

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace blas {

result<std::string> init_refblas() {
    if (g_handle) {
        return {};
    }

    std::string errmsg;

    for (auto const* path : {"libcblas.so", "libcblas.so.3"}) {
        if (!g_handle) {
            g_handle = try_dlopen(path, errmsg);
        }
    }

    if (!g_handle) {
        return error::make_errorf("cannot find BLAS library: {}", errmsg);
    }

    return init_cblas<runtime::cblas_interface_32>(g_handle);
}

result<std::string> init_lapack_refblas() {
    std::string errmsg;

    for (auto const* path : {"liblapacke.so", "liblapacke.so.3"}) {
        if (!g_lapacke) {
            g_handle = try_dlopen(path, errmsg);
        }
    }

    if (!g_lapacke) {
        return error::make_errorf("cannot find LAPACKE library: {}", errmsg);
    }

    return init_lapacke<runtime::lapacke_interface_32>(g_lapacke);
}

}  // namespace blas

CHARM_SYCL_END_NAMESPACE
