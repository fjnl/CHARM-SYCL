#include "hip_interface.hpp"
#include <iostream>
#include <iterator>
#include <dlfcn.h>
#include "../error.hpp"
#include "../format.hpp"

using namespace CHARM_SYCL_NS::error;

namespace {

template <class F>
static result<void> load_func(void* handle, F& fn, char const* name) {
    fn = reinterpret_cast<F>(dlsym(handle, name));

    if (!fn) {
        return make_errorf("HIP Error: function not found: {}", name);
    }

    return {};
}

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

uint32_t (*hip_interface_40200000::hip_drv_memcpy3d_async_ptr)(void const*, void*);
uint32_t (*hip_interface_40200000::hip_free_ptr)(void*);
char const* (*hip_interface_40200000::hip_get_error_string_ptr)(uint32_t);
uint32_t (*hip_interface_40200000::hip_init_ptr)(uint32_t);
uint32_t (*hip_interface_40200000::hip_malloc_ptr)(void*, uint64_t);
uint32_t (*hip_interface_40200000::hip_memcpy2d_ptr)(void*, uint64_t, void const*, uint64_t,
                                                     uint64_t, uint64_t, uint32_t);
uint32_t (*hip_interface_40200000::hip_memcpy_dtod_async_ptr)(void*, void*, uint64_t, void*);
uint32_t (*hip_interface_40200000::hip_memcpy_dtoh_async_ptr)(void*, void*, uint64_t, void*);
uint32_t (*hip_interface_40200000::hip_memcpy_htod_async_ptr)(void*, void*, uint64_t, void*);
uint32_t (*hip_interface_40200000::hip_memset_async_ptr)(void*, int, size_t, void*);
uint32_t (*hip_interface_40200000::hip_module_get_function_ptr)(void*, void*, char const*);
uint32_t (*hip_interface_40200000::hip_module_launch_kernel_ptr)(void*, uint32_t, uint32_t,
                                                                 uint32_t, uint32_t, uint32_t,
                                                                 uint32_t, uint32_t, void*,
                                                                 void*, void*);
uint32_t (*hip_interface_40200000::hip_module_load_data_ptr)(void*, void const*);
uint32_t (*hip_interface_40200000::hip_module_unload_ptr)(void*);
uint32_t (*hip_interface_40200000::hip_stream_create_ptr)(void*);
uint32_t (*hip_interface_40200000::hip_stream_destroy_ptr)(void*);
uint32_t (*hip_interface_40200000::hip_stream_synchronize_ptr)(void*);
uint32_t (*hip_interface_40200000::hip_get_device_properties_ptr)(void*, int);

void* hip_interface_40200000::handle_;

result<void> hip_interface_40200000::init() {
    if (handle_) {
        return {};
    }

    std::string errmsg;

    handle_ = dlopen("libamdhip64.so", RTLD_NOW);
    if (!handle_) {
        errmsg += dlerror();

        handle_ = dlopen("/opt/rocm/lib/libamdhip64.so", RTLD_NOW);
        if (!handle_) {
            errmsg += '\n';
            errmsg += dlerror();

            return make_errorf("HIP Error: cannot open the HIP driver: {}", errmsg);
        }
    }

    uint32_t (*get_version)(int*);
    load_func(handle_, get_version, "hipDriverGetVersion");
    int ver = -1;
    if (get_version(&ver) != 0) {
        dlclose(std::exchange(handle_, nullptr));
        return make_errorf("HIP Error: cannot retieve the HIP driver version");
    }

    if (!(MIN_VERSION <= ver && ver < MAX_VERSION)) {
        dlclose(std::exchange(handle_, nullptr));
        return make_errorf("HIP Error: unsupported HIP version: {}", ver);
    }

    CHECK_ERROR(load_func(handle_, hip_drv_memcpy3d_async_ptr, "hipDrvMemcpy3DAsync"));
    CHECK_ERROR(load_func(handle_, hip_free_ptr, "hipFree"));
    CHECK_ERROR(load_func(handle_, hip_get_error_string_ptr, "hipGetErrorString"));
    CHECK_ERROR(load_func(handle_, hip_init_ptr, "hipInit"));
    CHECK_ERROR(load_func(handle_, hip_malloc_ptr, "hipMalloc"));
    CHECK_ERROR(load_func(handle_, hip_memcpy2d_ptr, "hipMemcpy2D"));
    CHECK_ERROR(load_func(handle_, hip_memcpy_dtod_async_ptr, "hipMemcpyDtoDAsync"));
    CHECK_ERROR(load_func(handle_, hip_memcpy_dtoh_async_ptr, "hipMemcpyDtoHAsync"));
    CHECK_ERROR(load_func(handle_, hip_memcpy_htod_async_ptr, "hipMemcpyHtoDAsync"));
    CHECK_ERROR(load_func(handle_, hip_memset_async_ptr, "hipMemsetAsync"));
    CHECK_ERROR(load_func(handle_, hip_module_get_function_ptr, "hipModuleGetFunction"));
    CHECK_ERROR(load_func(handle_, hip_module_launch_kernel_ptr, "hipModuleLaunchKernel"));
    CHECK_ERROR(load_func(handle_, hip_module_load_data_ptr, "hipModuleLoadData"));
    CHECK_ERROR(load_func(handle_, hip_module_unload_ptr, "hipModuleUnload"));
    CHECK_ERROR(load_func(handle_, hip_stream_create_ptr, "hipStreamCreate"));
    CHECK_ERROR(load_func(handle_, hip_stream_destroy_ptr, "hipStreamDestroy"));
    CHECK_ERROR(load_func(handle_, hip_stream_synchronize_ptr, "hipStreamSynchronize"));
    CHECK_ERROR(load_func(handle_, hip_get_device_properties_ptr, "hipGetDeviceProperties"));

    return {};
}

void hip_interface_40200000::close() {
    hip_drv_memcpy3d_async_ptr = nullptr;
    hip_free_ptr = nullptr;
    hip_get_error_string_ptr = nullptr;
    hip_init_ptr = nullptr;
    hip_malloc_ptr = nullptr;
    hip_memcpy2d_ptr = nullptr;
    hip_memcpy_dtod_async_ptr = nullptr;
    hip_memcpy_dtoh_async_ptr = nullptr;
    hip_memcpy_htod_async_ptr = nullptr;
    hip_memset_async_ptr = nullptr;
    hip_module_get_function_ptr = nullptr;
    hip_module_launch_kernel_ptr = nullptr;
    hip_module_load_data_ptr = nullptr;
    hip_module_unload_ptr = nullptr;
    hip_stream_create_ptr = nullptr;
    hip_stream_destroy_ptr = nullptr;
    hip_stream_synchronize_ptr = nullptr;
    hip_get_device_properties_ptr = nullptr;

    if (auto h = std::exchange(handle_, nullptr)) {
        dlclose(h);
    }
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
