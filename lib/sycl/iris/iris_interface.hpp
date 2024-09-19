#pragma once

#include <charm/sycl/config.hpp>
#include "../error.hpp"
#include "../interfaces.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct iris_interface_20000 {
    using this_type = iris_interface_20000;

    static error::result<void> init();
    static void close();

    using kernel_t = detail::tagged_t<this_type, detail::record_type<16, 8>,
                                      detail::tag_name("iris_kernel")>;
    using mem_t =
        detail::tagged_t<this_type, detail::record_type<16, 8>, detail::tag_name("iris_mem")>;
    using task_t =
        detail::tagged_t<this_type, detail::record_type<16, 8>, detail::tag_name("iris_task")>;

    static constexpr auto SUCCESS = int(0);
    static constexpr auto depend = int(1 << 19);
    static constexpr auto data = int(1 << 20);
    static constexpr auto random = int(1 << 22);
    static constexpr auto ftf = int(33554432);
    static constexpr auto sdq = int(16777216);
    static constexpr auto cpu = int(64);
    static constexpr auto fpga = int(2048);
    static constexpr auto gpu = int(896);
    static constexpr auto name = int(4099);
    static constexpr auto r = int(-1);
    static constexpr auto roundrobin = int(262144);
    static constexpr auto rw = int(-3);
    static constexpr auto task_time_end = int(7);
    static constexpr auto task_time_start = int(6);
    static constexpr auto task_time_submit = int(5);
    static constexpr auto type = int(4100);
    static constexpr auto vendor = int(4098);
    static constexpr auto w = int(-2);

private:
    static int32_t (*iris_device_count_ptr)(void*);

public:
    static auto iris_device_count(int32_t* param0) {
        return detail::wrap<int32_t>(iris_device_count_ptr(detail::unwrap(param0)));
    }

private:
    static int32_t (*iris_device_info_ptr)(int32_t, int32_t, void*, void*);

public:
    static auto iris_device_info(int32_t param0, int32_t param1, void* param2, size_t* param3) {
        return detail::wrap<int32_t>(
            iris_device_info_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                 detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static int32_t (*iris_env_set_ptr)(void const*, void const*);

public:
    static auto iris_env_set(char const* param0, char const* param1) {
        return detail::wrap<int32_t>(
            iris_env_set_ptr(detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static int32_t (*iris_finalize_ptr)();

public:
    static auto iris_finalize() {
        return detail::wrap<int32_t>(iris_finalize_ptr());
    }

private:
    static int32_t (*iris_init_ptr)(void*, void*, int32_t);

public:
    static auto iris_init(int32_t* param0, char*** param1, int32_t param2) {
        return detail::wrap<int32_t>(iris_init_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2)));
    }

private:
    static int32_t (*iris_kernel_create_ptr)(void const*, void*);

public:
    static auto iris_kernel_create(char const* param0, kernel_t* param1) {
        return detail::wrap<int32_t>(
            iris_kernel_create_ptr(detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static int32_t (*iris_kernel_setarg_ptr)(typename kernel_t::native, int32_t, size_t, void*);

public:
    static auto iris_kernel_setarg(kernel_t param0, int32_t param1, size_t param2,
                                   void* param3) {
        return detail::wrap<int32_t>(
            iris_kernel_setarg_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                   detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static int32_t (*iris_kernel_setmem_off_ptr)(typename kernel_t::native, int32_t,
                                                 typename mem_t::native, size_t, size_t);

public:
    static auto iris_kernel_setmem_off(kernel_t param0, int32_t param1, mem_t param2,
                                       size_t param3, size_t param4) {
        return detail::wrap<int32_t>(iris_kernel_setmem_off_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4)));
    }

private:
    static int32_t (*iris_mem_create_ptr)(size_t, void*);

public:
    static auto iris_mem_create(size_t param0, mem_t* param1) {
        return detail::wrap<int32_t>(
            iris_mem_create_ptr(detail::unwrap(param0), detail::unwrap(param1)));
    }

private:
    static int32_t (*iris_platform_info_ptr)(int32_t, int32_t, void*, void*);

public:
    static auto iris_platform_info(int32_t param0, int32_t param1, void* param2,
                                   size_t* param3) {
        return detail::wrap<int32_t>(
            iris_platform_info_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                   detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static int32_t (*iris_synchronize_ptr)();

public:
    static auto iris_synchronize() {
        return detail::wrap<int32_t>(iris_synchronize_ptr());
    }

private:
    static int32_t (*iris_task_create_ptr)(void*);

public:
    static auto iris_task_create(task_t* param0) {
        return detail::wrap<int32_t>(iris_task_create_ptr(detail::unwrap(param0)));
    }

private:
    static int32_t (*iris_task_d2h_ptr)(typename task_t::native, typename mem_t::native, size_t,
                                        size_t, void*);

public:
    static auto iris_task_d2h(task_t param0, mem_t param1, size_t param2, size_t param3,
                              void* param4) {
        return detail::wrap<int32_t>(iris_task_d2h_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4)));
    }

private:
    static int32_t (*iris_task_depend_ptr)(typename task_t::native, int32_t, void*);

public:
    static auto iris_task_depend(task_t param0, int32_t param1, task_t* param2) {
        return detail::wrap<int32_t>(iris_task_depend_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2)));
    }

private:
    static int32_t (*iris_task_h2d_ptr)(typename task_t::native, typename mem_t::native, size_t,
                                        size_t, void*);

public:
    static auto iris_task_h2d(task_t param0, mem_t param1, size_t param2, size_t param3,
                              void* param4) {
        return detail::wrap<int32_t>(iris_task_h2d_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4)));
    }

private:
    static int32_t (*iris_task_cmd_reset_mem_ptr)(typename task_t::native,
                                                  typename mem_t::native, uint8_t);

public:
    static auto iris_task_cmd_reset_mem(task_t param0, mem_t param1, uint8_t param2) {
        return detail::wrap<int32_t>(iris_task_cmd_reset_mem_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2)));
    }

private:
    static int32_t (*iris_task_info_ptr)(typename task_t::native, int32_t, void*, void*);

public:
    static auto iris_task_info(task_t param0, int32_t param1, void* param2, size_t* param3) {
        return detail::wrap<int32_t>(
            iris_task_info_ptr(detail::unwrap(param0), detail::unwrap(param1),
                               detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static int32_t (*iris_task_kernel_object_ptr)(typename task_t::native,
                                                  typename kernel_t::native, int32_t, void*,
                                                  void*, void*);

public:
    static auto iris_task_kernel_object(task_t param0, kernel_t param1, int32_t param2,
                                        size_t* param3, size_t* param4, size_t* param5) {
        return detail::wrap<int32_t>(iris_task_kernel_object_ptr(
            detail::unwrap(param0), detail::unwrap(param1), detail::unwrap(param2),
            detail::unwrap(param3), detail::unwrap(param4), detail::unwrap(param5)));
    }

private:
    static int32_t (*iris_task_release_ptr)(typename task_t::native);

public:
    static auto iris_task_release(task_t param0) {
        return detail::wrap<int32_t>(iris_task_release_ptr(detail::unwrap(param0)));
    }

private:
    static void (*iris_task_retain_ptr)(typename task_t::native, uint8_t);

public:
    static auto iris_task_retain(task_t param0, uint8_t param1) {
        iris_task_retain_ptr(detail::unwrap(param0), detail::unwrap(param1));
    }

private:
    static int32_t (*iris_task_submit_ptr)(typename task_t::native, int32_t, void const*,
                                           int32_t);

public:
    static auto iris_task_submit(task_t param0, int32_t param1, char const* param2,
                                 int32_t param3) {
        return detail::wrap<int32_t>(
            iris_task_submit_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                 detail::unwrap(param2), detail::unwrap(param3)));
    }

private:
    static int32_t (*iris_mem_release_ptr)(typename mem_t::native);

public:
    static auto iris_mem_release(mem_t param0) {
        return iris_mem_release_ptr(detail::unwrap(param0));
    }

private:
    static int32_t (*iris_data_mem_create_ptr)(typename mem_t::native*, void*, size_t);

public:
    static auto iris_data_mem_create(mem_t* param0, void* param1, size_t param2) {
        return iris_data_mem_create_ptr(detail::unwrap(param0), detail::unwrap(param1),
                                        detail::unwrap(param2));
    }

private:
    static int32_t (*iris_task_dmem_flush_out_ptr)(typename task_t::native,
                                                   typename mem_t::native);

public:
    static auto iris_task_dmem_flush_out(task_t param0, mem_t param1) {
        return iris_task_dmem_flush_out_ptr(detail::unwrap(param0), detail::unwrap(param1));
    }

private:
    static int32_t (*iris_data_mem_update_ptr)(typename mem_t::native, void*);

public:
    static auto iris_data_mem_update(mem_t param0, void* param1) {
        return iris_data_mem_update_ptr(detail::unwrap(param0), param1);
    }

private:
    static void* handle_;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
