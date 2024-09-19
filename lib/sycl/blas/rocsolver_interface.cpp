#include <blas/rocsolver_interface.hpp>
#include <dlfcn.h>

namespace {
void* handle = nullptr;
}

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime {

error::result<std::string> rocsolver_interface_40200000::init() {
    handle = dlopen("librocsolver.so", RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        return error::make_errorf("Cannot find rocSOLVER library: {}", dlerror());
    }

    CHECK_ERROR(init_(handle));

    return version();
}

void rocsolver_interface_40200000::close() {
    close_();
    dlclose(std::exchange(handle, nullptr));
}

std::string rocsolver_interface_40200000::version() {
    size_t version_len;
    rocsolver_get_version_string_size(&version_len);

    std::vector<char> version_str(version_len + 1);
    rocsolver_get_version_string(version_str.data(), version_str.size());

    return version_str.data();
}

}  // namespace runtime
CHARM_SYCL_END_NAMESPACE
