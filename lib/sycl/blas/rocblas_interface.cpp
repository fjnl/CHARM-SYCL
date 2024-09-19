#include <blas/rocblas_interface.hpp>
#include <dlfcn.h>

namespace {
void* handle = nullptr;
}

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime {

error::result<std::string> rocblas_interface_40200000::init() {
    handle = dlopen("librocblas.so", RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        return error::make_errorf("Cannot find rocBLAS library: {}", dlerror());
    }

    CHECK_ERROR(init_(handle));

    return version();
}

void rocblas_interface_40200000::close() {
    close_();
    dlclose(std::exchange(handle, nullptr));
}

std::string rocblas_interface_40200000::version() {
    size_t version_len;
    rocblas_get_version_string_size(&version_len);

    std::vector<char> version_str(version_len + 1);
    rocblas_get_version_string(version_str.data(), version_str.size());

    return version_str.data();
}

}  // namespace runtime
CHARM_SYCL_END_NAMESPACE
