include(CheckCXXSymbolExists)

add_library(openmp_loader_obj OBJECT iris/openmp.cpp)
set_target_properties(openmp_loader_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(openmp_loader_obj PRIVATE sycl-headers Iris)
target_include_directories(openmp_loader_obj PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/thread-pool/include")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/blas)

function(run_generator)
    cmake_parse_arguments(opt "" "SCRIPT;OUTPUT" "OPTIONS;INPUTS" ${ARGN})

    cmake_path(ABSOLUTE_PATH opt_SCRIPT BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    cmake_path(ABSOLUTE_PATH opt_OUTPUT BASE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    cmake_path(GET opt_OUTPUT PARENT_PATH outdir)
    file(MAKE_DIRECTORY ${outdir})

    set(inputs)

    foreach(path ${opt_INPUTS})
        cmake_path(ABSOLUTE_PATH path BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        list(APPEND inputs ${path})
    endforeach()

    set(opt_INPUTS ${inputs})

    add_custom_command(
        OUTPUT
        ${opt_OUTPUT}
        COMMAND
        env PYTHONPATH=${PROJECT_SOURCE_DIR}/internal/python
        ${Python_EXECUTABLE}
        ${opt_SCRIPT}
        ${opt_OPTIONS}
        ${opt_OUTPUT}
        ${opt_INPUTS}
        DEPENDS
        ${opt_SCRIPT}
        ${opt_INPUTS}
        ${PROJECT_SOURCE_DIR}/internal/python/template.py
        ${PROJECT_SOURCE_DIR}/internal/python/blas_config.py
    )
endfunction()

function(if_gen input output)
    cmake_path(IS_ABSOLUTE input input_is_absolute)

    if(input_is_absolute)
        cmake_path(GET input PARENT_PATH basedir)
    else()
        set(basedir ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    cmake_path(ABSOLUTE_PATH output BASE_DIRECTORY ${basedir})

    run_generator(SCRIPT if_gen.py OUTPUT ${output} INPUTS ${input} OPTIONS ${ARGN})
endfunction()

# Common
set(BLAS1 ${PROJECT_SOURCE_DIR}/include/charm/sycl/runtime/blas1.ini)
set(BLAS2 ${PROJECT_SOURCE_DIR}/include/charm/sycl/runtime/blas2.ini)
set(BLAS3 ${PROJECT_SOURCE_DIR}/include/charm/sycl/runtime/blas3.ini)
set(LAPACK ${PROJECT_SOURCE_DIR}/include/charm/sycl/runtime/lapack.ini)
run_generator(SCRIPT blas/desc_gen.py OUTPUT blas/descs.hpp INPUTS ${BLAS1} ${BLAS2} ${BLAS3} ${LAPACK})
run_generator(SCRIPT blas/desc_gen.py OPTIONS --cpp OUTPUT blas/descs.cpp INPUTS ${BLAS1} ${BLAS2} ${BLAS3} ${LAPACK})

# CBLAS
run_generator(SCRIPT blas/cblas_ifgen.py OUTPUT blas/cblas_interface_32.xml INPUTS ${BLAS1} ${BLAS2} ${BLAS3} blas/cblas.xml OPTIONS cblas)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/cblas_interface_32.xml cblas_interface_32.hpp --thin-init)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/cblas_interface_32.xml cblas_interface_32_.cpp --cpp cblas_interface_32.hpp)
run_generator(SCRIPT blas/cblas_gen.py OUTPUT blas/cblas1.cpp INPUTS ${BLAS1} OPTIONS 1)
run_generator(SCRIPT blas/cblas_gen.py OUTPUT blas/cblas2.cpp INPUTS ${BLAS2} OPTIONS 2)
run_generator(SCRIPT blas/cblas_gen.py OUTPUT blas/cblas3.cpp INPUTS ${BLAS3} OPTIONS 3)

# LAPACKE
run_generator(SCRIPT blas/cblas_ifgen.py OUTPUT blas/lapacke_interface_32.xml INPUTS ${LAPACK} blas/lapacke.xml OPTIONS lapacke)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/lapacke_interface_32.xml lapacke_interface_32.hpp --thin-init)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/lapacke_interface_32.xml lapacke_interface_32_.cpp --cpp lapacke_interface_32.hpp)
run_generator(SCRIPT blas/cblas_gen.py OUTPUT blas/lapacke.cpp INPUTS ${LAPACK} OPTIONS lapacke)

# cuBLAS
run_generator(SCRIPT blas/cublas_ifgen.py OUTPUT blas/cublas_interface.xml INPUTS ${BLAS1} ${BLAS2} ${BLAS3} blas/cublas_11000.xml)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/cublas_interface.xml cublas_interface.hpp)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/cublas_interface.xml cublas_interface_.cpp --cpp cublas_interface.hpp)
run_generator(SCRIPT blas/cublas_gen.py OUTPUT blas/cublas1.cpp INPUTS ${BLAS1} OPTIONS 1)
run_generator(SCRIPT blas/cublas_gen.py OUTPUT blas/cublas2.cpp INPUTS ${BLAS2} OPTIONS 2)
run_generator(SCRIPT blas/cublas_gen.py OUTPUT blas/cublas3.cpp INPUTS ${BLAS3} OPTIONS 3)

# cuSOLVER
run_generator(SCRIPT blas/cusolver_ifgen.py OUTPUT blas/cusolver_interface.xml INPUTS ${LAPACK} blas/cusolver_11000.xml)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/cusolver_interface.xml cusolver_interface.hpp)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/cusolver_interface.xml cusolver_interface_.cpp --cpp cusolver_interface.hpp)
run_generator(SCRIPT blas/cusolver_gen.py OUTPUT blas/cusolver_lapack.cpp INPUTS ${LAPACK})

# rocBLAS
run_generator(SCRIPT blas/rocblas_ifgen.py OUTPUT blas/rocblas_interface.xml INPUTS ${BLAS1} ${BLAS2} ${BLAS3} blas/rocblas_40200000.xml)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/rocblas_interface.xml rocblas_interface.hpp)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/rocblas_interface.xml rocblas_interface_.cpp --cpp rocblas_interface.hpp)
run_generator(SCRIPT blas/rocblas_gen.py OUTPUT blas/rocblas1.cpp INPUTS ${BLAS1} OPTIONS 1)
run_generator(SCRIPT blas/rocblas_gen.py OUTPUT blas/rocblas2.cpp INPUTS ${BLAS2} OPTIONS 2)
run_generator(SCRIPT blas/rocblas_gen.py OUTPUT blas/rocblas3.cpp INPUTS ${BLAS3} OPTIONS 3)

# rocSOLVER
run_generator(SCRIPT blas/rocsolver_ifgen.py OUTPUT blas/rocsolver_interface.xml INPUTS ${LAPACK} blas/rocsolver_40200000.xml)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/rocsolver_interface.xml rocsolver_interface.hpp)
if_gen(${CMAKE_CURRENT_BINARY_DIR}/blas/rocsolver_interface.xml rocsolver_interface_.cpp --cpp rocsolver_interface.hpp)
run_generator(SCRIPT blas/rocsolver_gen.py OUTPUT blas/rocsolver_lapack.cpp INPUTS ${LAPACK})

set(arch ${BOOST_CONTEXT_ARCHITECTURE}_${BOOST_CONTEXT_ABI})

if(BOOST_CONTEXT_BINARY_FORMAT STREQUAL mach-o)
    set(arch ${arch}_macho)
else()
    set(arch ${arch}_${BOOST_CONTEXT_BINARY_FORMAT})
endif()

add_library(
    sycl

    accessor.cpp
    blas/blas.cpp
    buffer.cpp
    context.cpp
    dep.cpp
    dev_rts_cpu.cpp
    dev_rts.cpp
    dev_rts/coarse_task.cpp
    dev_rts/task.cpp
    device.cpp
    event.cpp
    fiber.cpp
    handler.cpp
    kreg.cpp
    local_accessor.cpp
    logging.cpp
    platform.cpp
    queue.cpp
    rts.cpp
    vec.cpp

    # CUDA backend
    cuda/dev_rts_cuda.cpp
    cuda/cuda_interface.cpp

    # HIP backend
    hip/dev_rts_hip.cpp
    hip/hip_interface.cpp

    # IRIS backend
    iris/iris_rts.cpp
    iris/iris_interface.cpp
    iris/dmem_rts.cpp

    # BLAS Common
    ${CMAKE_CURRENT_BINARY_DIR}/blas/descs.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/descs.hpp

    # CBLAS
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cblas_interface_32_.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cblas_interface_32.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cblas1.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cblas2.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cblas3.cpp
    blas/cblas.cpp

    # LAPACKe
    ${CMAKE_CURRENT_BINARY_DIR}/blas/lapacke_interface_32_.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/lapacke_interface_32.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/lapacke.cpp

    # CPU BLAS
    blas/mkl.cpp
    blas/openblas.cpp
    blas/refblas.cpp

    # cuBLAS
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cublas_interface_.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cublas_interface.hpp
    blas/cublas_interface.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cublas1.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cublas2.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cublas3.cpp
    blas/cublas.cpp

    # cuSOLVER
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cusolver_interface_.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cusolver_interface.hpp
    blas/cusolver_interface.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/cusolver_lapack.cpp
    blas/cusolver_lapack.cpp

    # rocBLAS
    ${CMAKE_CURRENT_BINARY_DIR}/blas/rocblas_interface_.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/rocblas_interface.hpp
    blas/rocblas_interface.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/rocblas1.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/rocblas2.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/rocblas3.cpp
    blas/rocblas.cpp

    # rocSOLVER
    ${CMAKE_CURRENT_BINARY_DIR}/blas/rocsolver_interface_.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/rocsolver_interface.hpp
    blas/rocsolver_interface.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/blas/rocsolver_lapack.cpp

    # Boost.Context
    # TODO more architectures and ABIs
    ${PROJECT_SOURCE_DIR}/vendor/boost/context/src/posix/stack_traits.cpp
    ${PROJECT_SOURCE_DIR}/vendor/boost/context/src/asm/make_${arch}_gas.S
    ${PROJECT_SOURCE_DIR}/vendor/boost/context/src/asm/jump_${arch}_gas.S
    ${PROJECT_SOURCE_DIR}/vendor/boost/context/src/asm/ontop_${arch}_gas.S
)

target_link_libraries(
    sycl
    PUBLIC
    sycl-headers
    Threads::Threads
    dl
    PRIVATE
    Boost::context
    "$<$<BOOL:${CHARM_SYCL_ENABLE_OPENBLAS}>:${OPENBLAS_TARGET} ${LAPACKE_TARGET}>"
)

target_compile_definitions(
    sycl
    PRIVATE
    "$<$<BOOL:${CHARM_SYCL_ENABLE_BLAS}>:CHARM_SYCL_ENABLE_BLAS>"
    "$<$<BOOL:${CHARM_SYCL_ENABLE_IRIS}>:CHARM_SYCL_ENABLE_IRIS>"
    "$<$<BOOL:${CHARM_SYCL_ENABLE_LAPACK}>:CHARM_SYCL_ENABLE_LAPACK>"
    "$<$<BOOL:${CHARM_SYCL_ENABLE_LAPACK}>:lapack_complex_double=std::complex<double$<ANGLE-R>>"
    "$<$<BOOL:${CHARM_SYCL_ENABLE_LAPACK}>:lapack_complex_float=std::complex<float$<ANGLE-R>>"
    "$<$<BOOL:${CHARM_SYCL_ENABLE_OPENBLAS}>:CHARM_SYCL_ENABLE_OPENBLAS>"
    "$<$<BOOL:${CHARM_SYCL_PORTABLE_RUNTIME}>:CHARM_SYCL_PORTABLE_RUNTIME>"
)

if(CSCC_PORTABLE_MODE)
    target_compile_definitions(sycl PRIVATE -DCSCC_PORTABLE_MODE -DCSCC_USE_STD_FORMAT)
else()
    include(CheckIncludeFileCXX)
    CHECK_INCLUDE_FILE_CXX("<format>" HAS_STD_FORMAT)

    if(HAS_STD_FORMAT)
        target_compile_definitions(sycl PRIVATE -DCSCC_USE_STD_FORMAT)
    else()
        target_compile_definitions(sycl PRIVATE -DCSCC_USE_FMT_FORMAT)
        target_link_libraries(sycl PRIVATE fmt::fmt-header-only)
    endif()
endif()

if(CUDAToolkit_FOUND)
    target_compile_definitions(sycl PRIVATE -DCHARM_SYCL_ENABLE_CUDA)

    if(NOT CSCC_PORTABLE_MODE)
        target_link_libraries(sycl PRIVATE CUDA::cuda_driver CUDA::cublas CUDA::cusolver)
    endif()
endif()

if(hip_FOUND)
    target_compile_definitions(sycl PRIVATE -DCHARM_SYCL_ENABLE_HIP)

    if(NOT CSCC_PORTABLE_MODE)
        find_package(rocblas REQUIRED)
        find_package(rocsolver REQUIRED)

        target_link_libraries(sycl PRIVATE hip::host roc::rocblas roc::rocsolver)
    endif()
endif()

if(TARGET Iris)
    target_compile_definitions(sycl PRIVATE -DCHARM_SYCL_ENABLE_IRIS)

    if(NOT CSCC_PORTABLE_MODE)
        target_link_libraries(sycl PRIVATE Iris)
    endif()
endif()

check_cxx_symbol_exists(get_nprocs "sys/sysinfo.h" HAVE_GET_NPROCS)

if(HAVE_GET_NPROCS)
    target_compile_definitions(sycl PRIVATE -DHAVE_GET_NPROCS)
endif()

if(DEFINED CHARM_SYCL_ENABLE_LOGGING AND CHARM_SYCL_ENABLE_LOGGING)
    target_compile_definitions(sycl PRIVATE -DCHARM_SYCL_ENABLE_LOGGING)
endif()

target_include_directories(
    sycl
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/thread-pool/include"
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/generated/lib/sycl
)

if(CSCC_PORTABLE_MODE)
    if(DEFINED CHARM_SYCL_RUNTIME_STDLIB AND CHARM_SYCL_RUNTIME_STDLIB)
        message(FATAL_ERROR "CHARM_SYCL_RUNTIME_STDLIB must not be disabled if CSCC_PORTABLE_MODE is enabled")
    endif()
else()
    if(DEFINED CHARM_SYCL_RUNTIME_STDLIB AND NOT CHARM_SYCL_RUNTIME_STDLIB STREQUAL "")
        foreach(target sycl openmp_loader_obj)
            target_compile_options(${target} PRIVATE -stdlib=${CHARM_SYCL_RUNTIME_STDLIB})
            target_link_options(${target} PRIVATE -stdlib=${CHARM_SYCL_RUNTIME_STDLIB})
        endforeach()
    endif()
endif()

if(NOT CSCC_PORTABLE_MODE)
    install(FILES "$<TARGET_OBJECTS:openmp_loader_obj>" DESTINATION "lib")
    install(TARGETS sycl DESTINATION "lib")
endif()
