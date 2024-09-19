#ifndef __HIP_PLATFORM_AMD__
#    define __HIP_PLATFORM_AMD__
#endif
#define HIP_INCLUDE_HIP_CHANNEL_DESCRIPTOR_H
#include <hip/hip_runtime_api.h>

template <int Version>
void begin_interfaces() {}

void end_interfaces() {}

template <class T>
void type_interface(void (*)(T), char const*, char const*) {}

template <class F>
void func_interface(F, char const*, char const*) {}

template <class T>
void constant_interface(T, char const*) {}

#define STR_(x) #x
#define STR(x) STR_(x)

#define TYPE(ty, name)            \
    extern void __dummy_##ty(ty); \
    type_interface(__dummy_##ty, #ty, name)

int main() {
    begin_interfaces<HIP_VERSION>();

    TYPE(hipDeviceptr_t, "deviceptr_t");
    TYPE(hipError_t, "error_t");
    TYPE(hipFunction_t, "function_t");
    TYPE(hipModule_t, "module_t");
    TYPE(hipStream_t, "stream_t");
    TYPE(hipMemoryType, "memorytype_t");
    TYPE(hipMemcpyKind, "memcpykind_t");
    TYPE(HIP_MEMCPY3D, "memcpy3d_t");

    constant_interface(hipSuccess, "Success");
    constant_interface(hipMemcpyHostToDevice, "MemcpyHostToDevice");
    constant_interface(hipMemcpyDeviceToHost, "MemcpyDeviceToHost");
    constant_interface(hipMemcpyDeviceToDevice, "MemcpyDeviceToDevice");
    constant_interface(hipMemoryTypeHost, "MemoryTypeHost");
    constant_interface(hipMemoryTypeDevice, "MemoryTypeDevice");

    func_interface(hipDrvMemcpy3DAsync, "hipDrvMemcpy3DAsync", STR(hipDrvMemcpy3DAsync));
    func_interface(hipFree, "hipFree", STR(hipFree));
    func_interface(hipGetErrorString, "hipGetErrorString", STR(hipGetErrorString));
    func_interface(hipInit, "hipInit", STR(hipInit));
    func_interface(static_cast<hipError_t (*)(void**, size_t)>(hipMalloc), "hipMalloc",
                   STR(hipMalloc));
    func_interface(hipMemcpy2D, "hipMemcpy2D", STR(hipMemcpy2D));
    func_interface(hipMemcpyDtoDAsync, "hipMemcpyDtoDAsync", STR(hipMemcpyDtoDAsync));
    func_interface(hipMemcpyDtoHAsync, "hipMemcpyDtoHAsync", STR(hipMemcpyDtoHAsync));
    func_interface(hipMemcpyHtoDAsync, "hipMemcpyHtoDAsync", STR(hipMemcpyHtoDAsync));
    func_interface(hipModuleGetFunction, "hipModuleGetFunction", STR(hipModuleGetFunction));
    func_interface(hipModuleLaunchKernel, "hipModuleLaunchKernel", STR(hipModuleLaunchKernel));
    func_interface(hipModuleLoadData, "hipModuleLoadData", STR(hipModuleLoadData));
    func_interface(hipModuleUnload, "hipModuleUnload", STR(hipModuleUnload));
    func_interface(hipStreamCreate, "hipStreamCreate", STR(hipStreamCreate));
    func_interface(hipStreamDestroy, "hipStreamDestroy", STR(hipStreamDestroy));
    func_interface(hipStreamSynchronize, "hipStreamSynchronize", STR(hipStreamSynchronize));

    end_interfaces();
    return 0;
}
