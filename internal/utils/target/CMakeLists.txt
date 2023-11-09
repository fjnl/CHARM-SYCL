add_library(utils-target OBJECT target.cpp)
target_include_directories(utils-target PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(CUDAToolkit_FOUND)
    target_compile_definitions(utils-target PRIVATE -DCHARM_SYCL_HAVE_NVIDIA_CUDA)
endif()

if(hip_FOUND)
    target_compile_definitions(utils-target PRIVATE -DCHARM_SYCL_HAVE_AMD_HIP)
endif()
