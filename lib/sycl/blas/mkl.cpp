#include <blas/cblas_interface_32.hpp>
#include <blas/lapacke_interface_32.hpp>
#include "common.hpp"

namespace {

void* g_handle = nullptr;

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace blas {

result<std::string> init_mkl() {
    if (g_handle) {
        return {};
    }

    std::string errmsg;

    for (auto const* path : {"libmkl_rt.so"}) {
        if (!g_handle) {
            g_handle = try_dlopen(path, errmsg);
        }
    }

    if (!g_handle) {
        return error::make_errorf("cannot find BLAS library: {}", errmsg);
    }

    return init_cblas<runtime::cblas_interface_32>(g_handle);
}

result<std::string> init_lapack_mkl() {
    return init_lapacke<runtime::lapacke_interface_32>(g_handle);
}

}  // namespace blas

CHARM_SYCL_END_NAMESPACE
