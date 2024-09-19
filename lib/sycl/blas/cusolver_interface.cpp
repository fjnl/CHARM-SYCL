#include <blas/cusolver_interface.hpp>
#include <dlfcn.h>

namespace {
void* handle = nullptr;
}

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime {

error::result<std::string> cusolver_interface_11000::init() {
    handle = dlopen("libcusolver.so", RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        return error::make_errorf("Cannot find cuSOLVER library: {}", dlerror());
    }

    return init_(handle);
}

std::string cusolver_interface_11000::version() {
    int ver;
    if (auto const err = cusolver_get_version(&ver); err) {
        // return error::make_errorf("cuSOLVER Error: cannot get the libray version: err={}",
        //                           *err);
        return "";
    }

    return format::format("{}.{}.{}", (ver / 10000) % 100, (ver / 100) % 100, ver % 100);
}

void cusolver_interface_11000::close() {
    close_();
}

}  // namespace runtime
CHARM_SYCL_END_NAMESPACE
