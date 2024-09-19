#include "iris_interface.hpp"
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
        return make_errorf("IRIS Error: function not found: {}", name);
    }

    return {};
}

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

int32_t (*iris_interface_20000::iris_device_count_ptr)(void*);
int32_t (*iris_interface_20000::iris_device_info_ptr)(int32_t, int32_t, void*, void*);
int32_t (*iris_interface_20000::iris_env_set_ptr)(void const*, void const*);
int32_t (*iris_interface_20000::iris_finalize_ptr)();
int32_t (*iris_interface_20000::iris_init_ptr)(void*, void*, int32_t);
int32_t (*iris_interface_20000::iris_kernel_create_ptr)(void const*, void*);
int32_t (*iris_interface_20000::iris_kernel_setarg_ptr)(typename kernel_t::native, int32_t,
                                                        size_t, void*);
int32_t (*iris_interface_20000::iris_kernel_setmem_off_ptr)(typename kernel_t::native, int32_t,
                                                            typename mem_t::native, size_t,
                                                            size_t);
int32_t (*iris_interface_20000::iris_mem_create_ptr)(size_t, void*);
int32_t (*iris_interface_20000::iris_platform_info_ptr)(int32_t, int32_t, void*, void*);
int32_t (*iris_interface_20000::iris_synchronize_ptr)();
int32_t (*iris_interface_20000::iris_task_create_ptr)(void*);
int32_t (*iris_interface_20000::iris_task_d2h_ptr)(typename task_t::native,
                                                   typename mem_t::native, size_t, size_t,
                                                   void*);
int32_t (*iris_interface_20000::iris_task_depend_ptr)(typename task_t::native, int32_t, void*);
int32_t (*iris_interface_20000::iris_task_h2d_ptr)(typename task_t::native,
                                                   typename mem_t::native, size_t, size_t,
                                                   void*);
int32_t (*iris_interface_20000::iris_task_cmd_reset_mem_ptr)(typename task_t::native,
                                                             typename mem_t::native, uint8_t);
int32_t (*iris_interface_20000::iris_task_info_ptr)(typename task_t::native, int32_t, void*,
                                                    void*);
int32_t (*iris_interface_20000::iris_task_kernel_object_ptr)(typename task_t::native,
                                                             typename kernel_t::native, int32_t,
                                                             void*, void*, void*);
int32_t (*iris_interface_20000::iris_task_release_ptr)(typename task_t::native);
void (*iris_interface_20000::iris_task_retain_ptr)(typename task_t::native, uint8_t);
int32_t (*iris_interface_20000::iris_task_submit_ptr)(typename task_t::native, int32_t,
                                                      void const*, int32_t);
int32_t (*iris_interface_20000::iris_mem_release_ptr)(typename mem_t::native);
int32_t (*iris_interface_20000::iris_data_mem_create_ptr)(typename mem_t::native*, void*,
                                                          size_t);
int32_t (*iris_interface_20000::iris_task_dmem_flush_out_ptr)(typename task_t::native,
                                                              typename mem_t::native);
int32_t (*iris_interface_20000::iris_data_mem_update_ptr)(typename mem_t::native, void*);

void* iris_interface_20000::handle_;

result<void> iris_interface_20000::init() {
    if (handle_) {
        return {};
    }

    std::string errmsg;

    handle_ = dlopen("libiris.so", RTLD_NOW);
    if (!handle_) {
        return make_errorf("IRIS Error: cannot open the IRIS driver: {}", dlerror());
    }

    CHECK_ERROR(load_func(handle_, iris_device_count_ptr, "iris_device_count"));
    CHECK_ERROR(load_func(handle_, iris_device_info_ptr, "iris_device_info"));
    CHECK_ERROR(load_func(handle_, iris_env_set_ptr, "iris_env_set"));
    CHECK_ERROR(load_func(handle_, iris_finalize_ptr, "iris_finalize"));
    CHECK_ERROR(load_func(handle_, iris_init_ptr, "iris_init"));
    CHECK_ERROR(load_func(handle_, iris_kernel_create_ptr, "iris_kernel_create"));
    CHECK_ERROR(load_func(handle_, iris_kernel_setarg_ptr, "iris_kernel_setarg"));
    CHECK_ERROR(load_func(handle_, iris_kernel_setmem_off_ptr, "iris_kernel_setmem_off"));
    CHECK_ERROR(load_func(handle_, iris_mem_create_ptr, "iris_mem_create"));
    CHECK_ERROR(load_func(handle_, iris_platform_info_ptr, "iris_platform_info"));
    CHECK_ERROR(load_func(handle_, iris_synchronize_ptr, "iris_synchronize"));
    CHECK_ERROR(load_func(handle_, iris_task_create_ptr, "iris_task_create"));
    CHECK_ERROR(load_func(handle_, iris_task_d2h_ptr, "iris_task_d2h"));
    CHECK_ERROR(load_func(handle_, iris_task_depend_ptr, "iris_task_depend"));
    CHECK_ERROR(load_func(handle_, iris_task_h2d_ptr, "iris_task_h2d"));
    CHECK_ERROR(load_func(handle_, iris_task_cmd_reset_mem_ptr, "iris_task_cmd_reset_mem"));
    CHECK_ERROR(load_func(handle_, iris_task_info_ptr, "iris_task_info"));
    CHECK_ERROR(load_func(handle_, iris_task_kernel_object_ptr, "iris_task_kernel_object"));
    CHECK_ERROR(load_func(handle_, iris_task_release_ptr, "iris_task_release"));
    CHECK_ERROR(load_func(handle_, iris_task_retain_ptr, "iris_task_retain"));
    CHECK_ERROR(load_func(handle_, iris_task_submit_ptr, "iris_task_submit"));
    CHECK_ERROR(load_func(handle_, iris_mem_release_ptr, "iris_mem_release"));
    CHECK_ERROR(load_func(handle_, iris_data_mem_create_ptr, "iris_data_mem_create"));
    CHECK_ERROR(load_func(handle_, iris_task_dmem_flush_out_ptr, "iris_task_dmem_flush_out"));
    CHECK_ERROR(load_func(handle_, iris_data_mem_update_ptr, "iris_data_mem_update"));

    return {};
}

void iris_interface_20000::close() {
    iris_device_count_ptr = nullptr;
    iris_device_info_ptr = nullptr;
    iris_env_set_ptr = nullptr;
    iris_finalize_ptr = nullptr;
    iris_init_ptr = nullptr;
    iris_kernel_create_ptr = nullptr;
    iris_kernel_setarg_ptr = nullptr;
    iris_kernel_setmem_off_ptr = nullptr;
    iris_mem_create_ptr = nullptr;
    iris_platform_info_ptr = nullptr;
    iris_synchronize_ptr = nullptr;
    iris_task_create_ptr = nullptr;
    iris_task_d2h_ptr = nullptr;
    iris_task_depend_ptr = nullptr;
    iris_task_h2d_ptr = nullptr;
    iris_task_cmd_reset_mem_ptr = nullptr;
    iris_task_info_ptr = nullptr;
    iris_task_kernel_object_ptr = nullptr;
    iris_task_release_ptr = nullptr;
    iris_task_retain_ptr = nullptr;
    iris_task_submit_ptr = nullptr;
    iris_task_dmem_flush_out_ptr = nullptr;
    iris_data_mem_update_ptr = nullptr;

    if (auto h = std::exchange(handle_, nullptr)) {
        dlclose(h);
    }
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
