
#pragma once
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <error.hpp>
#include <interfaces.hpp>
CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime {
struct cuda_interface {
    using this_type = cuda_interface;
    static error::result<void> init();
    static void close();
    static void clear();
    static std::string version_str();

private:
    struct impl;
    static std::unique_ptr<impl> pimpl_;
    template <size_t Offset, class R, class T>
    [[maybe_unused]]
    static inline void set_(R& record, T val) {
        *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(record.address()) + Offset) = val;
    }
    template <size_t Offset, class T, class R>
    [[maybe_unused]]
    static inline auto get_(R const& record) {
        return *reinterpret_cast<T const*>(
            reinterpret_cast<std::byte const*>(record.address()) + Offset);
    }

public:
    using context_t = detail::tagged_t<this_type, void*, detail::tag_name("CUcontext")>;
    using device_t = detail::tagged_t<this_type, int32_t, detail::tag_name("CUdevice"),
                                      detail::init_val(-2)>;
    using deviceptr_t = detail::tagged_t<this_type, uint64_t, detail::tag_name("CUdeviceptr")>;
    using function_t = detail::tagged_t<this_type, void*, detail::tag_name("CUfunction")>;
    using module_t = detail::tagged_t<this_type, void*, detail::tag_name("CUmodule")>;
    using stream_t = detail::tagged_t<this_type, void*, detail::tag_name("CUstream")>;
    using result_t = detail::tagged_t<this_type, int32_t, detail::tag_name("CUresult")>;
    using memcpy2d_t = detail::tagged_t<this_type, detail::record_type<128, 8>,
                                        detail::tag_name("CUDA_MEMCPY2D")>;
    using memcpy3d_t = detail::tagged_t<this_type, detail::record_type<200, 8>,
                                        detail::tag_name("CUDA_MEMCPY3D")>;
    using memorytype_t = detail::tagged_t<this_type, int32_t, detail::tag_name("CUmemorytype")>;
    using datatype_t = detail::tagged_t<this_type, int32_t, detail::tag_name("cudaDataType")>;
    using dev_attr_t =
        detail::tagged_t<this_type, int32_t, detail::tag_name("CUdevice_attribute")>;
    static constexpr auto k_CUDA_SUCCESS = result_t(0);
    static constexpr auto k_MEMORYTYPE_HOST = memorytype_t(1);
    static constexpr auto k_MEMORYTYPE_DEVICE = memorytype_t(2);
    static constexpr auto k_R_32F = datatype_t(0);
    static constexpr auto k_C_32F = datatype_t(4);
    static constexpr auto k_R_64F = datatype_t(1);
    static constexpr auto k_C_64F = datatype_t(5);
    static constexpr auto k_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT = dev_attr_t(16);
    static inline void set_srcXInBytes(memcpy2d_t& x, uint64_t val) {
        set_<0>(x, val);
    }
    static inline auto get_srcXInBytes(memcpy2d_t const& x) {
        return get_<0, uint64_t>(x);
    }
    static inline void set_srcY(memcpy2d_t& x, uint64_t val) {
        set_<8>(x, val);
    }
    static inline auto get_srcY(memcpy2d_t const& x) {
        return get_<8, uint64_t>(x);
    }
    static inline void set_srcMemoryType(memcpy2d_t& x, memorytype_t val) {
        set_<16>(x, val);
    }
    static inline auto get_srcMemoryType(memcpy2d_t const& x) {
        return get_<16, memorytype_t>(x);
    }
    static inline void set_srcHost(memcpy2d_t& x, void const* val) {
        set_<24>(x, val);
    }
    static inline auto get_srcHost(memcpy2d_t const& x) {
        return get_<24, void const*>(x);
    }
    static inline void set_srcDevice(memcpy2d_t& x, deviceptr_t val) {
        set_<32>(x, val);
    }
    static inline auto get_srcDevice(memcpy2d_t const& x) {
        return get_<32, deviceptr_t>(x);
    }
    static inline void set_srcPitch(memcpy2d_t& x, uint64_t val) {
        set_<48>(x, val);
    }
    static inline auto get_srcPitch(memcpy2d_t const& x) {
        return get_<48, uint64_t>(x);
    }
    static inline void set_dstXInBytes(memcpy2d_t& x, uint64_t val) {
        set_<56>(x, val);
    }
    static inline auto get_dstXInBytes(memcpy2d_t const& x) {
        return get_<56, uint64_t>(x);
    }
    static inline void set_dstY(memcpy2d_t& x, uint64_t val) {
        set_<64>(x, val);
    }
    static inline auto get_dstY(memcpy2d_t const& x) {
        return get_<64, uint64_t>(x);
    }
    static inline void set_dstMemoryType(memcpy2d_t& x, memorytype_t val) {
        set_<72>(x, val);
    }
    static inline auto get_dstMemoryType(memcpy2d_t const& x) {
        return get_<72, memorytype_t>(x);
    }
    static inline void set_dstHost(memcpy2d_t& x, void* val) {
        set_<80>(x, val);
    }
    static inline auto get_dstHost(memcpy2d_t const& x) {
        return get_<80, void*>(x);
    }
    static inline void set_dstDevice(memcpy2d_t& x, deviceptr_t val) {
        set_<88>(x, val);
    }
    static inline auto get_dstDevice(memcpy2d_t const& x) {
        return get_<88, deviceptr_t>(x);
    }
    static inline void set_dstPitch(memcpy2d_t& x, uint64_t val) {
        set_<104>(x, val);
    }
    static inline auto get_dstPitch(memcpy2d_t const& x) {
        return get_<104, uint64_t>(x);
    }
    static inline void set_WidthInBytes(memcpy2d_t& x, uint64_t val) {
        set_<112>(x, val);
    }
    static inline auto get_WidthInBytes(memcpy2d_t const& x) {
        return get_<112, uint64_t>(x);
    }
    static inline void set_Height(memcpy2d_t& x, uint64_t val) {
        set_<120>(x, val);
    }
    static inline auto get_Height(memcpy2d_t const& x) {
        return get_<120, uint64_t>(x);
    }
    static inline void set_srcXInBytes(memcpy3d_t& x, uint64_t val) {
        set_<0>(x, val);
    }
    static inline auto get_srcXInBytes(memcpy3d_t const& x) {
        return get_<0, uint64_t>(x);
    }
    static inline void set_srcY(memcpy3d_t& x, uint64_t val) {
        set_<8>(x, val);
    }
    static inline auto get_srcY(memcpy3d_t const& x) {
        return get_<8, uint64_t>(x);
    }
    static inline void set_srcZ(memcpy3d_t& x, uint64_t val) {
        set_<16>(x, val);
    }
    static inline auto get_srcZ(memcpy3d_t const& x) {
        return get_<16, uint64_t>(x);
    }
    static inline void set_srcLOD(memcpy3d_t& x, uint64_t val) {
        set_<24>(x, val);
    }
    static inline auto get_srcLOD(memcpy3d_t const& x) {
        return get_<24, uint64_t>(x);
    }
    static inline void set_srcMemoryType(memcpy3d_t& x, memorytype_t val) {
        set_<32>(x, val);
    }
    static inline auto get_srcMemoryType(memcpy3d_t const& x) {
        return get_<32, memorytype_t>(x);
    }
    static inline void set_srcHost(memcpy3d_t& x, void const* val) {
        set_<40>(x, val);
    }
    static inline auto get_srcHost(memcpy3d_t const& x) {
        return get_<40, void const*>(x);
    }
    static inline void set_srcDevice(memcpy3d_t& x, deviceptr_t val) {
        set_<48>(x, val);
    }
    static inline auto get_srcDevice(memcpy3d_t const& x) {
        return get_<48, deviceptr_t>(x);
    }
    static inline void set_srcPitch(memcpy3d_t& x, uint64_t val) {
        set_<72>(x, val);
    }
    static inline auto get_srcPitch(memcpy3d_t const& x) {
        return get_<72, uint64_t>(x);
    }
    static inline void set_srcHeight(memcpy3d_t& x, uint64_t val) {
        set_<80>(x, val);
    }
    static inline auto get_srcHeight(memcpy3d_t const& x) {
        return get_<80, uint64_t>(x);
    }
    static inline void set_dstXInBytes(memcpy3d_t& x, uint64_t val) {
        set_<88>(x, val);
    }
    static inline auto get_dstXInBytes(memcpy3d_t const& x) {
        return get_<88, uint64_t>(x);
    }
    static inline void set_dstY(memcpy3d_t& x, uint64_t val) {
        set_<96>(x, val);
    }
    static inline auto get_dstY(memcpy3d_t const& x) {
        return get_<96, uint64_t>(x);
    }
    static inline void set_dstZ(memcpy3d_t& x, uint64_t val) {
        set_<104>(x, val);
    }
    static inline auto get_dstZ(memcpy3d_t const& x) {
        return get_<104, uint64_t>(x);
    }
    static inline void set_dstLOD(memcpy3d_t& x, uint64_t val) {
        set_<112>(x, val);
    }
    static inline auto get_dstLOD(memcpy3d_t const& x) {
        return get_<112, uint64_t>(x);
    }
    static inline void set_dstMemoryType(memcpy3d_t& x, memorytype_t val) {
        set_<120>(x, val);
    }
    static inline auto get_dstMemoryType(memcpy3d_t const& x) {
        return get_<120, memorytype_t>(x);
    }
    static inline void set_dstHost(memcpy3d_t& x, void* val) {
        set_<128>(x, val);
    }
    static inline auto get_dstHost(memcpy3d_t const& x) {
        return get_<128, void*>(x);
    }
    static inline void set_dstDevice(memcpy3d_t& x, deviceptr_t val) {
        set_<136>(x, val);
    }
    static inline auto get_dstDevice(memcpy3d_t const& x) {
        return get_<136, deviceptr_t>(x);
    }
    static inline void set_dstPitch(memcpy3d_t& x, uint64_t val) {
        set_<160>(x, val);
    }
    static inline auto get_dstPitch(memcpy3d_t const& x) {
        return get_<160, uint64_t>(x);
    }
    static inline void set_dstHeight(memcpy3d_t& x, uint64_t val) {
        set_<168>(x, val);
    }
    static inline auto get_dstHeight(memcpy3d_t const& x) {
        return get_<168, uint64_t>(x);
    }
    static inline void set_WidthInBytes(memcpy3d_t& x, uint64_t val) {
        set_<176>(x, val);
    }
    static inline auto get_WidthInBytes(memcpy3d_t const& x) {
        return get_<176, uint64_t>(x);
    }
    static inline void set_Height(memcpy3d_t& x, uint64_t val) {
        set_<184>(x, val);
    }
    static inline auto get_Height(memcpy3d_t const& x) {
        return get_<184, uint64_t>(x);
    }
    static inline void set_Depth(memcpy3d_t& x, uint64_t val) {
        set_<192>(x, val);
    }
    static inline auto get_Depth(memcpy3d_t const& x) {
        return get_<192, uint64_t>(x);
    }

private:
    static void* cu_ctx_pop_current_ptr;

public:
    static inline auto cu_ctx_pop_current(context_t* param0) {
        using Fn = typename result_t::native (*)(typename context_t::native*);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_ctx_pop_current_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cu_ctx_set_current_ptr;

public:
    static inline auto cu_ctx_set_current(context_t param0) {
        using Fn = typename result_t::native (*)(typename context_t::native);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_ctx_set_current_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cu_device_get_ptr;

public:
    static inline auto cu_device_get(device_t* param0, int32_t param1) {
        using Fn = typename result_t::native (*)(typename device_t::native*, int32_t);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_device_get_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_device_primary_ctx_release_ptr;

public:
    static inline auto cu_device_primary_ctx_release(device_t param0) {
        using Fn = typename result_t::native (*)(typename device_t::native);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_device_primary_ctx_release_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cu_device_primary_ctx_retain_ptr;

public:
    static inline auto cu_device_primary_ctx_retain(context_t* param0, device_t param1) {
        using Fn = typename result_t::native (*)(typename context_t::native*,
                                                 typename device_t::native);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_device_primary_ctx_retain_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_device_primary_ctx_set_flags_ptr;

public:
    static inline auto cu_device_primary_ctx_set_flags(device_t param0, uint32_t param1) {
        using Fn = typename result_t::native (*)(typename device_t::native, uint32_t);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_device_primary_ctx_set_flags_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_get_error_string_ptr;

public:
    static inline auto cu_get_error_string(result_t param0, char const** param1) {
        using Fn = typename result_t::native (*)(typename result_t::native, char const**);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_get_error_string_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_init_ptr;

public:
    static inline auto cu_init(uint32_t param0) {
        using Fn = typename result_t::native (*)(uint32_t);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_init_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cu_launch_kernel_ptr;

public:
    static inline auto cu_launch_kernel(function_t param0, uint32_t param1, uint32_t param2,
                                        uint32_t param3, uint32_t param4, uint32_t param5,
                                        uint32_t param6, uint32_t param7, stream_t param8,
                                        void** param9, void** param10) {
        using Fn = typename result_t::native (*)(
            typename function_t::native, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
            uint32_t, uint32_t, typename stream_t::native, void**, void**);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_launch_kernel_ptr)(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4), detail::unwrap(param5),
            detail::unwrap(param6), detail::unwrap(param7), detail::unwrap(param8),
            detail::unwrap(param9), detail::unwrap(param10)));
    }

private:
    static void* cu_mem_alloc_ptr;

public:
    static inline auto cu_mem_alloc(deviceptr_t* param0, size_t param1) {
        using Fn = typename result_t::native (*)(typename deviceptr_t::native*, size_t);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_mem_alloc_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_memcpy2d_async_ptr;

public:
    static inline auto cu_memcpy2d_async(memcpy2d_t const* param0, stream_t param1) {
        using Fn = typename result_t::native (*)(typename memcpy2d_t::native const*,
                                                 typename stream_t::native);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_memcpy2d_async_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_memcpy3d_async_ptr;

public:
    static inline auto cu_memcpy3d_async(memcpy3d_t const* param0, stream_t param1) {
        using Fn = typename result_t::native (*)(typename memcpy3d_t::native const*,
                                                 typename stream_t::native);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_memcpy3d_async_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_memcpy_dtod_async_ptr;

public:
    static inline auto cu_memcpy_dtod_async(deviceptr_t param0, deviceptr_t param1,
                                            size_t param2, stream_t param3) {
        using Fn = typename result_t::native (*)(typename deviceptr_t::native,
                                                 typename deviceptr_t::native, size_t,
                                                 typename stream_t::native);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_memcpy_dtod_async_ptr)(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3)));
    }

private:
    static void* cu_memcpy_dtoh_async_ptr;

public:
    static inline auto cu_memcpy_dtoh_async(void* param0, deviceptr_t param1, size_t param2,
                                            stream_t param3) {
        using Fn = typename result_t::native (*)(void*, typename deviceptr_t::native, size_t,
                                                 typename stream_t::native);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_memcpy_dtoh_async_ptr)(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3)));
    }

private:
    static void* cu_memcpy_htod_async_ptr;

public:
    static inline auto cu_memcpy_htod_async(deviceptr_t param0, void const* param1,
                                            size_t param2, stream_t param3) {
        using Fn = typename result_t::native (*)(typename deviceptr_t::native, void const*,
                                                 size_t, typename stream_t::native);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_memcpy_htod_async_ptr)(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3)));
    }

private:
    static void* cu_memset_d8_async_ptr;

public:
    static inline auto cu_memset_d8_async(deviceptr_t param0, uint8_t param1, size_t param2,
                                          stream_t param3) {
        using Fn = typename result_t::native (*)(typename deviceptr_t::native, uint8_t, size_t,
                                                 typename stream_t::native);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_memset_d8_async_ptr)(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3)));
    }

private:
    static void* cu_mem_free_ptr;

public:
    static inline auto cu_mem_free(deviceptr_t param0) {
        using Fn = typename result_t::native (*)(typename deviceptr_t::native);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_mem_free_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cu_module_get_function_ptr;

public:
    static inline auto cu_module_get_function(function_t* param0, module_t param1,
                                              char const* param2) {
        using Fn = typename result_t::native (*)(typename function_t::native*,
                                                 typename module_t::native, char const*);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_module_get_function_ptr)(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2)));
    }

private:
    static void* cu_module_load_fat_binary_ptr;

public:
    static inline auto cu_module_load_fat_binary(module_t* param0, void const* param1) {
        using Fn = typename result_t::native (*)(typename module_t::native*, void const*);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_module_load_fat_binary_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_module_unload_ptr;

public:
    static inline auto cu_module_unload(module_t param0) {
        using Fn = typename result_t::native (*)(typename module_t::native);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_module_unload_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cu_stream_create_ptr;

public:
    static inline auto cu_stream_create(stream_t* param0, uint32_t param1) {
        using Fn = typename result_t::native (*)(typename stream_t::native*, uint32_t);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_stream_create_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_stream_destroy_ptr;

public:
    static inline auto cu_stream_destroy(stream_t param0) {
        using Fn = typename result_t::native (*)(typename stream_t::native);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_stream_destroy_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cu_stream_synchronize_ptr;

public:
    static inline auto cu_stream_synchronize(stream_t param0) {
        using Fn = typename result_t::native (*)(typename stream_t::native);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_stream_synchronize_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cu_mem_alloc_host_ptr;

public:
    static inline auto cu_mem_alloc_host(void** param0, uint64_t param1) {
        using Fn = typename result_t::native (*)(void**, uint64_t);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_mem_alloc_host_ptr)(
            detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static void* cu_mem_free_host_ptr;

public:
    static inline auto cu_mem_free_host(void* param0) {
        using Fn = typename result_t::native (*)(void*);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_mem_free_host_ptr)(detail::unwrap(param0)));
    }

private:
    static void* cuDeviceGetAttribute_ptr;

public:
    static inline auto cuDeviceGetAttribute(int32_t* param0, dev_attr_t param1,
                                            device_t param2) {
        using Fn = typename result_t::native (*)(int32_t*, typename dev_attr_t::native,
                                                 typename device_t::native);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cuDeviceGetAttribute_ptr)(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2)));
    }

private:
    static void* cu_mem_host_register_ptr;

public:
    static inline auto cu_mem_host_register(void* param0, size_t param1, uint32_t param2) {
        using Fn = typename result_t::native (*)(void*, size_t, uint32_t);
        return detail::wrap<result_t>(reinterpret_cast<Fn>(cu_mem_host_register_ptr)(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2)));
    }

private:
    static void* cu_mem_host_unregister_ptr;

public:
    static inline auto cu_mem_host_unregister(void* param0) {
        using Fn = typename result_t::native (*)(void*);
        return detail::wrap<result_t>(
            reinterpret_cast<Fn>(cu_mem_host_unregister_ptr)(detail::unwrap(param0)));
    }
};
}  // namespace runtime
CHARM_SYCL_END_NAMESPACE
