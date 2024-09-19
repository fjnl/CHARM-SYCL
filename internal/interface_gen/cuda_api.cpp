#include <cuda.h>

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
    begin_interfaces<CUDA_VERSION>();

    TYPE(CUcontext, "context_t");
    TYPE(CUdevice, "device_t");
    TYPE(CUdeviceptr, "deviceptr_t");
    TYPE(CUfunction, "function_t");
    TYPE(CUmodule, "module_t");
    TYPE(CUresult, "result_t");
    TYPE(CUstream, "stream_t");
    TYPE(CUmemorytype, "memorytype_t");
    TYPE(CUDA_MEMCPY2D, "memcpy2d_t");
    TYPE(CUDA_MEMCPY3D, "memcpy3d_t");

    constant_interface(CUDA_SUCCESS, "SUCCESS");
    constant_interface(CU_MEMORYTYPE_HOST, "MEMORYTYPE_HOST");
    constant_interface(CU_MEMORYTYPE_DEVICE, "MEMORYTYPE_DEVICE");

    func_interface(cuCtxPopCurrent, "cuCtxPopCurrent", STR(cuCtxPopCurrent));
    func_interface(cuCtxSetCurrent, "cuCtxSetCurrent", STR(cuCtxSetCurrent));
    func_interface(cuDeviceGet, "cuDeviceGet", STR(cuDeviceGet));
    func_interface(cuDevicePrimaryCtxRelease, "cuDevicePrimaryCtxRelease",
                   STR(cuDevicePrimaryCtxRelease));
    func_interface(cuDevicePrimaryCtxRetain, "cuDevicePrimaryCtxRetain",
                   STR(cuDevicePrimaryCtxRetain));
    func_interface(cuDevicePrimaryCtxSetFlags, "cuDevicePrimaryCtxSetFlags",
                   STR(cuDevicePrimaryCtxSetFlags));
    func_interface(cuGetErrorString, "cuGetErrorString", STR(cuGetErrorString));
    func_interface(cuInit, "cuInit", STR(cuInit));
    func_interface(cuLaunchKernel, "cuLaunchKernel", STR(cuLaunchKernel));
    func_interface(cuMemAlloc, "cuMemAlloc", STR(cuMemAlloc));
    func_interface(cuMemcpy2DAsync, "cuMemcpy2DAsync", STR(cuMemcpy2DAsync));
    func_interface(cuMemcpy3DAsync, "cuMemcpy3DAsync", STR(cuMemcpy3DAsync));
    func_interface(cuMemcpyDtoDAsync, "cuMemcpyDtoDAsync", STR(cuMemcpyDtoDAsync));
    func_interface(cuMemcpyDtoHAsync, "cuMemcpyDtoHAsync", STR(cuMemcpyDtoHAsync));
    func_interface(cuMemcpyHtoDAsync, "cuMemcpyHtoDAsync", STR(cuMemcpyHtoDAsync));
    func_interface(cuMemFree, "cuMemFree", STR(cuMemFree));
    func_interface(cuModuleGetFunction, "cuModuleGetFunction", STR(cuModuleGetFunction));
    func_interface(cuModuleLoadFatBinary, "cuModuleLoadFatBinary", STR(cuModuleLoadFatBinary));
    func_interface(cuModuleUnload, "cuModuleUnload", STR(cuModuleUnload));
    func_interface(cuStreamCreate, "cuStreamCreate", STR(cuStreamCreate));
    func_interface(cuStreamDestroy, "cuStreamDestroy", STR(cuStreamDestroy));
    func_interface(cuStreamSynchronize, "cuStreamSynchronize", STR(cuStreamSynchronize));

    end_interfaces();
    return 0;
}
