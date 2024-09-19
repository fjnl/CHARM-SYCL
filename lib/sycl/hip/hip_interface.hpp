#pragma once

#include "../interfaces.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct hip_interface_40200000 {
    static error::result<void> init();
    static void close();

    using this_type = hip_interface_40200000;

    static constexpr int VERSION = 40200000;
    static constexpr int MIN_VERSION = 40200000;
    // static constexpr int MAX_VERSION = 50800000;
    static constexpr int MAX_VERSION = 60100000;

    using deviceptr_t = detail::tagged_t<this_type, void*, detail::tag_name("hipDeviceptr_t")>;
    using function_t = detail::tagged_t<this_type, void*, detail::tag_name("hipFunction_t")>;
    using module_t = detail::tagged_t<this_type, void*, detail::tag_name("hipModule_t")>;
    using stream_t = detail::tagged_t<this_type, void*, detail::tag_name("hipStream_t")>;
    using error_t = detail::tagged_t<this_type, uint32_t, detail::tag_name("hipError_t")>;
    using memorytype_t =
        detail::tagged_t<this_type, uint32_t, detail::tag_name("hipMemoryType")>;
    using memcpykind_t =
        detail::tagged_t<this_type, uint32_t, detail::tag_name("hipMemcpyKind")>;
    using memcpy3d_t = detail::tagged_t<this_type, detail::record_type<128, 8>,
                                        detail::tag_name("HIP_MEMCPY3D")>;
    using device_prop_t = detail::tagged_t<this_type, detail::record_type<792, 8>,
                                           detail::tag_name("hipDeviceProp_t")>;

    static constexpr auto k_Success = error_t(0);
    static constexpr auto k_MemcpyHostToDevice = memcpykind_t(1);
    static constexpr auto k_MemcpyDeviceToHost = memcpykind_t(2);
    static constexpr auto k_MemcpyDeviceToDevice = memcpykind_t(3);
    static constexpr auto k_MemoryTypeHost = memorytype_t(0);
    static constexpr auto k_MemoryTypeDevice = memorytype_t(1);

    static auto get_multiProcessorCount(device_prop_t const& record) {
        return get_<336, int>(record);
    }

    static void set_srcXInBytes(memcpy3d_t& record, uint32_t val) {
        set_<0>(record, val);
    }
    static void set_srcY(memcpy3d_t& record, uint32_t val) {
        set_<4>(record, val);
    }
    static void set_srcZ(memcpy3d_t& record, uint32_t val) {
        set_<8>(record, val);
    }
    static void set_srcLOD(memcpy3d_t& record, uint32_t val) {
        set_<12>(record, val);
    }
    static void set_srcMemoryType(memcpy3d_t& record, memorytype_t val) {
        set_<16>(record, val);
    }
    static void set_srcHost(memcpy3d_t& record, void const* val) {
        set_<24>(record, val);
    }
    static void set_srcDevice(memcpy3d_t& record, deviceptr_t val) {
        set_<32>(record, val);
    }
    static void set_srcPitch(memcpy3d_t& record, uint32_t val) {
        set_<48>(record, val);
    }
    static void set_srcHeight(memcpy3d_t& record, uint32_t val) {
        set_<52>(record, val);
    }
    static void set_dstXInBytes(memcpy3d_t& record, uint32_t val) {
        set_<56>(record, val);
    }
    static void set_dstY(memcpy3d_t& record, uint32_t val) {
        set_<60>(record, val);
    }
    static void set_dstZ(memcpy3d_t& record, uint32_t val) {
        set_<64>(record, val);
    }
    static void set_dstLOD(memcpy3d_t& record, uint32_t val) {
        set_<68>(record, val);
    }
    static void set_dstMemoryType(memcpy3d_t& record, memorytype_t val) {
        set_<72>(record, val);
    }
    static void set_dstHost(memcpy3d_t& record, void* val) {
        set_<80>(record, val);
    }
    static void set_dstDevice(memcpy3d_t& record, deviceptr_t val) {
        set_<88>(record, *val);
    }
    static void set_dstPitch(memcpy3d_t& record, uint32_t val) {
        set_<104>(record, val);
    }
    static void set_dstHeight(memcpy3d_t& record, uint32_t val) {
        set_<108>(record, val);
    }
    static void set_WidthInBytes(memcpy3d_t& record, uint32_t val) {
        set_<112>(record, val);
    }
    static void set_Height(memcpy3d_t& record, uint32_t val) {
        set_<116>(record, val);
    }
    static void set_Depth(memcpy3d_t& record, uint32_t val) {
        set_<120>(record, val);
    }

private:
    template <size_t Offset, class R, class T>
    static void set_(R& record, T val) {
        *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(record.address()) + Offset) = val;
    }

    template <size_t Offset, class T, class R>
    static T get_(R const& record) {
        return *reinterpret_cast<T const*>(
            reinterpret_cast<std::byte const*>(record.address()) + Offset);
    }

private:
    static uint32_t (*hip_drv_memcpy3d_async_ptr)(void const*, void*);

public:
    static auto hip_drv_memcpy3d_async(memcpy3d_t const* param0, stream_t param1) {
        return detail::wrap<error_t>(
            hip_drv_memcpy3d_async_ptr(detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static uint32_t (*hip_free_ptr)(void*);

public:
    static auto hip_free(deviceptr_t param0) {
        return detail::wrap<error_t>(hip_free_ptr(detail::unwrap(param0)));
    }

private:
    static char const* (*hip_get_error_string_ptr)(uint32_t);

public:
    static auto hip_get_error_string(error_t param0) {
        return hip_get_error_string_ptr(detail::unwrap(param0));
    }

private:
    static uint32_t (*hip_init_ptr)(uint32_t);

public:
    static auto hip_init(uint32_t param0) {
        return detail::wrap<error_t>(hip_init_ptr(detail::unwrap(param0)));
    }

private:
    static uint32_t (*hip_malloc_ptr)(void*, uint64_t);

public:
    static auto hip_malloc(deviceptr_t* param0, size_t param1) {
        return detail::wrap<error_t>(
            hip_malloc_ptr(detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static uint32_t (*hip_memcpy2d_ptr)(void*, uint64_t, void const*, uint64_t, uint64_t,
                                        uint64_t, uint32_t);

public:
    static auto hip_memcpy2d(void* param0, size_t param1, void const* param2, size_t param3,
                             size_t param4, size_t param5, memcpykind_t param6) {
        return detail::wrap<error_t>(hip_memcpy2d_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4), detail::unwrap(param5),
            detail::unwrap(param6)));
    }

    static auto hip_memcpy2d(deviceptr_t param0, size_t param1, void const* param2,
                             size_t param3, size_t param4, size_t param5, memcpykind_t param6) {
        return detail::wrap<error_t>(hip_memcpy2d_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4), detail::unwrap(param5),
            detail::unwrap(param6)));
    }

    static auto hip_memcpy2d(void* param0, size_t param1, deviceptr_t param2, size_t param3,
                             size_t param4, size_t param5, memcpykind_t param6) {
        return detail::wrap<error_t>(hip_memcpy2d_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4), detail::unwrap(param5),
            detail::unwrap(param6)));
    }

    static auto hip_memcpy2d(deviceptr_t param0, size_t param1, deviceptr_t param2,
                             size_t param3, size_t param4, size_t param5, memcpykind_t param6) {
        return detail::wrap<error_t>(hip_memcpy2d_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4), detail::unwrap(param5),
            detail::unwrap(param6)));
    }

private:
    static uint32_t (*hip_memcpy_dtod_async_ptr)(void*, void*, uint64_t, void*);

public:
    static auto hip_memcpy_dtod_async(deviceptr_t param0, deviceptr_t param1, size_t param2,
                                      stream_t param3) {
        return detail::wrap<error_t>(
            hip_memcpy_dtod_async_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                      detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static uint32_t (*hip_memcpy_dtoh_async_ptr)(void*, void*, uint64_t, void*);

public:
    static auto hip_memcpy_dtoh_async(void* param0, deviceptr_t param1, size_t param2,
                                      stream_t param3) {
        return detail::wrap<error_t>(
            hip_memcpy_dtoh_async_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                      detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static uint32_t (*hip_memcpy_htod_async_ptr)(void*, void*, uint64_t, void*);

public:
    static auto hip_memcpy_htod_async(deviceptr_t param0, void* param1, size_t param2,
                                      stream_t param3) {
        return detail::wrap<error_t>(
            hip_memcpy_htod_async_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                      detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static uint32_t (*hip_memset_async_ptr)(void*, int, size_t, void*);

public:
    static auto hip_memset_async(deviceptr_t param0, int param1, size_t param2,
                                 stream_t param3) {
        return detail::wrap<error_t>(
            hip_memset_async_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                 detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static uint32_t (*hip_module_get_function_ptr)(void*, void*, char const*);

public:
    static auto hip_module_get_function(function_t* param0, module_t param1,
                                        char const* param2) {
        return detail::wrap<error_t>(hip_module_get_function_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2)));
    }

private:
    static uint32_t (*hip_module_launch_kernel_ptr)(void*, uint32_t, uint32_t, uint32_t,
                                                    uint32_t, uint32_t, uint32_t, uint32_t,
                                                    void*, void*, void*);

public:
    static auto hip_module_launch_kernel(function_t param0, uint32_t param1, uint32_t param2,
                                         uint32_t param3, uint32_t param4, uint32_t param5,
                                         uint32_t param6, uint32_t param7, stream_t param8,
                                         void** param9, void** param10) {
        return detail::wrap<error_t>(hip_module_launch_kernel_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4), detail::unwrap(param5),
            detail::unwrap(param6), detail::unwrap(param7), detail::unwrap(param8),
            detail::unwrap(param9), detail::unwrap(param10)));
    }

private:
    static uint32_t (*hip_module_load_data_ptr)(void*, void const*);

public:
    static auto hip_module_load_data(module_t* param0, void const* param1) {
        return detail::wrap<error_t>(
            hip_module_load_data_ptr(detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static uint32_t (*hip_module_unload_ptr)(void*);

public:
    static auto hip_module_unload(module_t param0) {
        return detail::wrap<error_t>(hip_module_unload_ptr(detail::unwrap(param0)));
    }

private:
    static uint32_t (*hip_stream_create_ptr)(void*);

public:
    static auto hip_stream_create(stream_t* param0) {
        return detail::wrap<error_t>(hip_stream_create_ptr(detail::unwrap(param0)));
    }

private:
    static uint32_t (*hip_stream_destroy_ptr)(void*);

public:
    static auto hip_stream_destroy(stream_t param0) {
        return detail::wrap<error_t>(hip_stream_destroy_ptr(detail::unwrap(param0)));
    }

private:
    static uint32_t (*hip_stream_synchronize_ptr)(void*);

public:
    static auto hip_stream_synchronize(stream_t param0) {
        return detail::wrap<error_t>(hip_stream_synchronize_ptr(detail::unwrap(param0)));
    }

private:
    static uint32_t (*hip_get_device_properties_ptr)(void*, int);

public:
    static auto hip_get_device_properties(device_prop_t* param0, int param1) {
        return detail::wrap<error_t>(
            hip_get_device_properties_ptr(detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* handle_;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
