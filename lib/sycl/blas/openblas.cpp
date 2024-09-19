#include <blas/cblas_interface_32.hpp>
#include <blas/lapacke_interface_32.hpp>
#include "common.hpp"

namespace {

void* g_handle = nullptr;

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace blas {

result<std::string> init_openblas() {
    if (g_handle) {
        return {};
    }

    std::string errmsg;

    for (auto const* path : {"libopenblasp.so", "libopenblaso.so", "libopenblas.so"}) {
        if (!g_handle) {
            g_handle = try_dlopen(path, errmsg);
        }
    }

    if (!g_handle) {
        return error::make_errorf("cannot find BLAS library: {}", errmsg);
    }

    return init_cblas<sycl::runtime::cblas_interface_32>(g_handle);
}

result<std::string> init_lapack_openblas() {
    return init_lapacke<sycl::runtime::lapacke_interface_32>(g_handle);
}

}  // namespace blas

CHARM_SYCL_END_NAMESPACE
