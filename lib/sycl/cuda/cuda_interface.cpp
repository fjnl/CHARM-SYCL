#include <iostream>
#include <iterator>
#include <cuda/cuda_interface.hpp>
#include <dlfcn.h>
#include "../format.hpp"

using namespace CHARM_SYCL_NS::error;

namespace {

static result<void> load_func(void* handle, void*& fn, char const* name) {
    fn = dlsym(handle, name);

    if (!fn) {
        return make_errorf("CUDA Error: function not found: {}", name);
    }

    return {};
}

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

#include <cuda/cuda_interface_vars.ipp>

struct cuda_interface::impl {
    void* h;
};

std::unique_ptr<cuda_interface::impl> cuda_interface::pimpl_;

result<void> cuda_interface::init() {
    if (pimpl_) {
        return {};
    }

    pimpl_.reset(new impl);
    std::string errmsg;

    pimpl_->h = dlopen("libcuda.so", RTLD_NOW);
    if (!pimpl_->h) {
        errmsg += dlerror();

        pimpl_->h = dlopen("/usr/lib64/libcuda.so", RTLD_NOW);
        if (!pimpl_->h) {
            errmsg += '\n';
            errmsg += dlerror();

            return make_errorf("CUDA Error: cannot open the CUDA driver: {}", errmsg);
        }
    }

    CHECK_ERROR(load_func(pimpl_->h, cu_ctx_pop_current_ptr, "cuCtxPopCurrent_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_ctx_set_current_ptr, "cuCtxSetCurrent"));
    CHECK_ERROR(load_func(pimpl_->h, cu_device_get_ptr, "cuDeviceGet"));
    CHECK_ERROR(load_func(pimpl_->h, cu_device_primary_ctx_release_ptr,
                          "cuDevicePrimaryCtxRelease_v2"));
    CHECK_ERROR(
        load_func(pimpl_->h, cu_device_primary_ctx_retain_ptr, "cuDevicePrimaryCtxRetain"));
    CHECK_ERROR(load_func(pimpl_->h, cu_device_primary_ctx_set_flags_ptr,
                          "cuDevicePrimaryCtxSetFlags_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_get_error_string_ptr, "cuGetErrorString"));
    CHECK_ERROR(load_func(pimpl_->h, cu_init_ptr, "cuInit"));
    CHECK_ERROR(load_func(pimpl_->h, cu_launch_kernel_ptr, "cuLaunchKernel"));
    CHECK_ERROR(load_func(pimpl_->h, cu_mem_alloc_ptr, "cuMemAlloc_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_memcpy2d_async_ptr, "cuMemcpy2DAsync_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_memcpy3d_async_ptr, "cuMemcpy3DAsync_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_memcpy_dtod_async_ptr, "cuMemcpyDtoDAsync_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_memcpy_dtoh_async_ptr, "cuMemcpyDtoHAsync_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_memcpy_htod_async_ptr, "cuMemcpyHtoDAsync_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_memset_d8_async_ptr, "cuMemsetD8Async"));
    CHECK_ERROR(load_func(pimpl_->h, cu_mem_free_ptr, "cuMemFree_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_module_get_function_ptr, "cuModuleGetFunction"));
    CHECK_ERROR(load_func(pimpl_->h, cu_module_load_fat_binary_ptr, "cuModuleLoadFatBinary"));
    CHECK_ERROR(load_func(pimpl_->h, cu_module_unload_ptr, "cuModuleUnload"));
    CHECK_ERROR(load_func(pimpl_->h, cu_stream_create_ptr, "cuStreamCreate"));
    CHECK_ERROR(load_func(pimpl_->h, cu_stream_destroy_ptr, "cuStreamDestroy_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_stream_synchronize_ptr, "cuStreamSynchronize"));
    CHECK_ERROR(load_func(pimpl_->h, cu_mem_alloc_host_ptr, "cuMemAllocHost_v2"));
    CHECK_ERROR(load_func(pimpl_->h, cu_mem_free_host_ptr, "cuMemFreeHost"));
    CHECK_ERROR(load_func(pimpl_->h, cuDeviceGetAttribute_ptr, "cuDeviceGetAttribute"));
    CHECK_ERROR(load_func(pimpl_->h, cu_mem_host_register_ptr, "cuMemHostRegister"));
    CHECK_ERROR(load_func(pimpl_->h, cu_mem_host_unregister_ptr, "cuMemHostUnregister"));

    return {};
}

void cuda_interface::close() {
    clear();
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
