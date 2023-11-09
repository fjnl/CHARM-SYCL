enable_asan_if_debug()
include(CheckCXXSymbolExists)
enable_language(ASM)

if(CUDAToolkit_FOUND)
    list(APPEND extra dev_rts_cuda.cpp)
endif()

if(hip_FOUND)
    list(APPEND extra dev_rts_hip.cpp)
endif()

if(TARGET Iris)
    list(APPEND extra iris/iris_rts.cpp)

    add_library(openmp_loader_obj OBJECT iris/openmp.cpp)
    set_target_properties(openmp_loader_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(openmp_loader_obj PRIVATE sycl-headers Iris)
    target_include_directories(openmp_loader_obj PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/thread-pool")
    install(FILES "$<TARGET_OBJECTS:openmp_loader_obj>" DESTINATION "lib")
endif()

add_library(
    sycl

    accessor.cpp
    buffer.cpp
    context.cpp
    dep.cpp
    dev_rts_cpu.cpp
    dev_rts.cpp
    device.cpp
    event.cpp
    fiber.cpp
    handler.cpp
    kreg.cpp
    local_accessor.cpp
    platform.cpp
    queue.cpp
    rts.cpp

    # TODO more architectures and ABIs
    ${PROJECT_SOURCE_DIR}/vendor/boost/context/src/posix/stack_traits.cpp
    ${PROJECT_SOURCE_DIR}/vendor/boost/context/src/asm/make_x86_64_sysv_elf_gas.S
    ${PROJECT_SOURCE_DIR}/vendor/boost/context/src/asm/jump_x86_64_sysv_elf_gas.S
    ${PROJECT_SOURCE_DIR}/vendor/boost/context/src/asm/ontop_x86_64_sysv_elf_gas.S
    ${extra}
)

if(CUDAToolkit_FOUND)
    target_link_libraries(sycl PRIVATE CUDA::cuda_driver)
    target_compile_definitions(sycl PRIVATE -DHAVE_DEV_RTS_CUDA)
endif()

if(hip_FOUND)
    target_link_libraries(sycl PRIVATE hip::host)
    target_compile_definitions(sycl PRIVATE -DHAVE_DEV_RTS_HIP)
endif()

target_link_libraries(
    sycl
    PUBLIC
    sycl-headers
    Threads::Threads
    PRIVATE
    Boost::context
    fmt::fmt-header-only
    utils-logging
)

check_cxx_symbol_exists(get_nprocs "sys/sysinfo.h" HAVE_GET_NPROCS)

if(HAVE_GET_NPROCS)
    target_compile_definitions(sycl PRIVATE -DHAVE_GET_NPROCS)
endif()

if(TARGET Iris)
    target_compile_definitions(sycl PRIVATE -DHAVE_IRIS -DUSE_IRIS_DEFAULT)
    target_link_libraries(sycl PRIVATE Iris)

    check_cxx_symbol_exists(iris_task_time_submit "iris/iris.h" HAVE_IRIS_TASK_SUBMIT)

    if(HAVE_IRIS_TASK_SUBMIT)
        target_compile_definitions(sycl PRIVATE -DHAVE_IRIS_TASK_SUBMIT)
    endif()
endif()

if(DEFINED CHARM_SYCL_ENABLE_LOGGING AND CHARM_SYCL_ENABLE_LOGGING)
    target_compile_definitions(sycl PRIVATE -DCHARM_SYCL_ENABLE_LOGGING)
endif()

target_include_directories(sycl PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/thread-pool")

if(DEFINED CHARM_SYCL_RUNTIME_STDLIB AND NOT CHARM_SYCL_RUNTIME_STDLIB STREQUAL "")
    foreach(target sycl openmp_loader_obj)
        target_compile_options(${target} PRIVATE -stdlib=${CHARM_SYCL_RUNTIME_STDLIB})
        target_link_options(${target} PRIVATE -stdlib=${CHARM_SYCL_RUNTIME_STDLIB})
    endforeach()
endif()

install(TARGETS sycl DESTINATION "lib")
