#include <blas/cublas_interface.hpp>
#include <dlfcn.h>

namespace {
void* handle = nullptr;
}

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime {

error::result<std::string> cublas_interface_11000::init() {
    handle = dlopen("libcublas.so", RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        return error::make_errorf("Cannot find cuBLAS library: {}", dlerror());
    }

    CHECK_ERROR(init_(handle));

    return version();
}

void cublas_interface_11000::close() {
    close_();
    dlclose(std::exchange(handle, nullptr));
}

std::string cublas_interface_11000::version() {
    blas_t handle;
    result_t err;
    if (err = cublas_create(&handle); err) {
        return format::format("cuBLAS Error: cannot create a handle: {}",
                              cublas_get_status_string(err));
    }

    int ver;
    if (err = cublas_get_version(handle, &ver); err) {
        cublas_destroy(handle);
        return format::format("cuBLAS Error: cannot get the library version: {}",
                              cublas_get_status_string(err));
    }

    cublas_destroy(handle);

    return format::format("{}.{}.{}", (ver / 10000) % 100, (ver / 100) % 100, ver % 100);
}

}  // namespace runtime
CHARM_SYCL_END_NAMESPACE
