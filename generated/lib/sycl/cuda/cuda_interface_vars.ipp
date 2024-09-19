void* cuda_interface::cu_ctx_pop_current_ptr = nullptr;
void* cuda_interface::cu_ctx_set_current_ptr = nullptr;
void* cuda_interface::cu_device_get_ptr = nullptr;
void* cuda_interface::cu_device_primary_ctx_release_ptr = nullptr;
void* cuda_interface::cu_device_primary_ctx_retain_ptr = nullptr;
void* cuda_interface::cu_device_primary_ctx_set_flags_ptr = nullptr;
void* cuda_interface::cu_get_error_string_ptr = nullptr;
void* cuda_interface::cu_init_ptr = nullptr;
void* cuda_interface::cu_launch_kernel_ptr = nullptr;
void* cuda_interface::cu_mem_alloc_ptr = nullptr;
void* cuda_interface::cu_memcpy2d_async_ptr = nullptr;
void* cuda_interface::cu_memcpy3d_async_ptr = nullptr;
void* cuda_interface::cu_memcpy_dtod_async_ptr = nullptr;
void* cuda_interface::cu_memcpy_dtoh_async_ptr = nullptr;
void* cuda_interface::cu_memcpy_htod_async_ptr = nullptr;
void* cuda_interface::cu_memset_d8_async_ptr = nullptr;
void* cuda_interface::cu_mem_free_ptr = nullptr;
void* cuda_interface::cu_module_get_function_ptr = nullptr;
void* cuda_interface::cu_module_load_fat_binary_ptr = nullptr;
void* cuda_interface::cu_module_unload_ptr = nullptr;
void* cuda_interface::cu_stream_create_ptr = nullptr;
void* cuda_interface::cu_stream_destroy_ptr = nullptr;
void* cuda_interface::cu_stream_synchronize_ptr = nullptr;
void* cuda_interface::cu_mem_alloc_host_ptr = nullptr;
void* cuda_interface::cu_mem_free_host_ptr = nullptr;
void* cuda_interface::cuDeviceGetAttribute_ptr = nullptr;
void* cuda_interface::cu_mem_host_register_ptr = nullptr;
void* cuda_interface::cu_mem_host_unregister_ptr = nullptr;
void cuda_interface::clear() {
    cu_ctx_pop_current_ptr = nullptr;
    cu_ctx_set_current_ptr = nullptr;
    cu_device_get_ptr = nullptr;
    cu_device_primary_ctx_release_ptr = nullptr;
    cu_device_primary_ctx_retain_ptr = nullptr;
    cu_device_primary_ctx_set_flags_ptr = nullptr;
    cu_get_error_string_ptr = nullptr;
    cu_init_ptr = nullptr;
    cu_launch_kernel_ptr = nullptr;
    cu_mem_alloc_ptr = nullptr;
    cu_memcpy2d_async_ptr = nullptr;
    cu_memcpy3d_async_ptr = nullptr;
    cu_memcpy_dtod_async_ptr = nullptr;
    cu_memcpy_dtoh_async_ptr = nullptr;
    cu_memcpy_htod_async_ptr = nullptr;
    cu_memset_d8_async_ptr = nullptr;
    cu_mem_free_ptr = nullptr;
    cu_module_get_function_ptr = nullptr;
    cu_module_load_fat_binary_ptr = nullptr;
    cu_module_unload_ptr = nullptr;
    cu_stream_create_ptr = nullptr;
    cu_stream_destroy_ptr = nullptr;
    cu_stream_synchronize_ptr = nullptr;
    cu_mem_alloc_host_ptr = nullptr;
    cu_mem_free_host_ptr = nullptr;
    cuDeviceGetAttribute_ptr = nullptr;
    cu_mem_host_register_ptr = nullptr;
    cu_mem_host_unregister_ptr = nullptr;
    pimpl_.reset();
}
