#include <iris/iris.h>

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

#define FUNC(fn) func_interface(fn, #fn, STR(fn))

int main() {
    begin_interfaces<20000>();

    TYPE(iris_kernel, "kernel_t");
    TYPE(iris_mem, "mem_t");
    TYPE(iris_task, "task_t");

    constant_interface(IRIS_SUCCESS, "SUCCESS");
    constant_interface(iris_all, "all");
    constant_interface(iris_any, "any");
    constant_interface(iris_cpu, "cpu");
    constant_interface(iris_fpga, "fpga");
    constant_interface(iris_gpu, "gpu");
    constant_interface(iris_name, "name");
    constant_interface(iris_r, "r");
    constant_interface(iris_roundrobin, "roundrobin");
    constant_interface(iris_rw, "rw");
    constant_interface(iris_task_time_end, "task_time_end");
    constant_interface(iris_task_time_start, "task_time_start");
    constant_interface(iris_task_time_submit, "task_time_submit");
    constant_interface(iris_type, "type");
    constant_interface(iris_vendor, "vendor");
    constant_interface(iris_w, "w");

    FUNC(iris_device_count);
    FUNC(iris_device_info);
    FUNC(iris_env_set);
    FUNC(iris_finalize);
    FUNC(iris_init);
    FUNC(iris_kernel_create);
    FUNC(iris_kernel_setarg);
    FUNC(iris_kernel_setmem_off);
    FUNC(iris_mem_create);
    FUNC(iris_platform_info);
    FUNC(iris_synchronize);
    FUNC(iris_task_create);
    FUNC(iris_task_d2h);
    FUNC(iris_task_depend);
    FUNC(iris_task_h2d);
    FUNC(iris_task_info);
    FUNC(iris_task_kernel_object);
    FUNC(iris_task_release);
    FUNC(iris_task_retain);
    FUNC(iris_task_submit);

    end_interfaces();
    return 0;
}
