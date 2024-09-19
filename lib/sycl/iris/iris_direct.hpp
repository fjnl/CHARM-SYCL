#pragma once

#include <iris/iris.h>
#include <charm/sycl/config.hpp>
#include "../error.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct iris_direct {
    static error::result<void> init() {
        return {};
    }
    static void close() {}

    using kernel_t = iris_kernel;
    using mem_t = iris_mem;
    using task_t = iris_task;

    static constexpr auto SUCCESS = IRIS_SUCCESS;
    static constexpr auto ftf = iris_ftf;
    static constexpr auto sdq = iris_sdq;
    static constexpr auto cpu = iris_cpu;
    static constexpr auto fpga = iris_fpga;
    static constexpr auto gpu = iris_gpu;
    static constexpr auto name = iris_name;
    static constexpr auto r = iris_r;
    static constexpr auto roundrobin = iris_roundrobin;
    static constexpr auto rw = iris_rw;
    static constexpr auto task_time_end = iris_task_time_end;
    static constexpr auto task_time_start = iris_task_time_start;
    static constexpr auto task_time_submit = iris_task_time_submit;
    static constexpr auto type = iris_type;
    static constexpr auto vendor = iris_vendor;
    static constexpr auto w = iris_w;

    static auto iris_device_count(int32_t* param0) {
        return ::iris_device_count(param0);
    }

    static auto iris_device_info(int32_t param0, int32_t param1, void* param2, size_t* param3) {
        return ::iris_device_info(param0, param1, param2, param3);
    }

    static auto iris_env_set(char const* param0, char const* param1) {
        return ::iris_env_set(param0, param1);
    }

    static auto iris_finalize() {
        return ::iris_finalize();
    }

    static auto iris_init(int32_t* param0, char*** param1, int32_t param2) {
        return ::iris_init(param0, param1, param2);
    }

    static auto iris_kernel_create(char const* param0, kernel_t* param1) {
        return ::iris_kernel_create(param0, param1);
    }

    static auto iris_kernel_setarg(kernel_t param0, int32_t param1, size_t param2,
                                   void* param3) {
        return ::iris_kernel_setarg(param0, param1, param2, param3);
    }

    static auto iris_kernel_setmem_off(kernel_t param0, int32_t param1, mem_t param2,
                                       size_t param3, size_t param4) {
        return ::iris_kernel_setmem_off(param0, param1, param2, param3, param4);
    }

    static auto iris_mem_create(size_t param0, mem_t* param1) {
        return ::iris_mem_create(param0, param1);
    }

    static auto iris_platform_info(int32_t param0, int32_t param1, void* param2,
                                   size_t* param3) {
        return ::iris_platform_info(param0, param1, param2, param3);
    }

    static auto iris_synchronize() {
        return ::iris_synchronize();
    }

    static auto iris_task_create(task_t* param0) {
        return ::iris_task_create(param0);
    }

    static auto iris_task_d2h(task_t param0, mem_t param1, size_t param2, size_t param3,
                              void* param4) {
        return ::iris_task_d2h(param0, param1, param2, param3, param4);
    }

    static auto iris_task_depend(task_t param0, int32_t param1, task_t* param2) {
        return ::iris_task_depend(param0, param1, param2);
    }

    static auto iris_task_h2d(task_t param0, mem_t param1, size_t param2, size_t param3,
                              void* param4) {
        return ::iris_task_h2d(param0, param1, param2, param3, param4);
    }

    static auto iris_task_info(task_t param0, int32_t param1, void* param2, size_t* param3) {
        return ::iris_task_info(param0, param1, param2, param3);
    }

    static auto iris_task_kernel_object(task_t param0, kernel_t param1, int32_t param2,
                                        size_t* param3, size_t* param4, size_t* param5) {
        return ::iris_task_kernel_object(param0, param1, param2, param3, param4, param5);
    }

    static auto iris_task_release(task_t param0) {
        return ::iris_task_release(param0);
    }

    static auto iris_task_retain(task_t param0, uint8_t param1) {
        ::iris_task_retain(param0, param1);
    }

    static auto iris_task_submit(task_t param0, int32_t param1, char const* param2,
                                 int32_t param3) {
        return ::iris_task_submit(param0, param1, param2, param3);
    }
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
