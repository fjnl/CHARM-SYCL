cmake_minimum_required(VERSION 3.20)

project(charm-bench-vecadd LANGUAGES CXX VERSION 0.0.1)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not defined")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(VECADD_OPENMP)
    find_package(OpenMP REQUIRED)

    add_executable(vecadd-openmp main.cpp openmp.cpp)
    target_link_libraries(vecadd-openmp PRIVATE OpenMP::OpenMP_CXX)
    install(TARGETS vecadd-openmp DESTINATION ".")
elseif(VECADD_OPENACC)
    find_package(OpenACC REQUIRED)

    add_executable(vecadd-openacc main.cpp openacc.cpp)
    target_link_libraries(vecadd-openacc PRIVATE OpenACC::OpenACC_CXX)
    install(TARGETS vecadd-openacc DESTINATION ".")
elseif(VECADD_CUDA)
    find_package(CUDAToolkit REQUIRED)
    enable_language(CUDA)

    add_executable(vecadd-cuda main.cpp cuda.cu)
    target_link_libraries(vecadd-cuda PRIVATE CUDA::cudart_static)
    install(TARGETS vecadd-cuda DESTINATION ".")

    add_executable(vecadd-cuda-multi main.cpp cuda_multi.cu)
    target_link_libraries(vecadd-cuda-multi PRIVATE CUDA::cudart_static)
    install(TARGETS vecadd-cuda-multi DESTINATION ".")
elseif(VECADD_CSCC)
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        set(debug_opts -g)
    endif()

    add_custom_target(
        vecadd-sycl ALL
        ${CSCC_COMMAND}
        --targets=all
        ${debug_opts}
        -O3
        -o vecadd-sycl
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/sycl.cpp
        DEPENDS main.cpp sycl.cpp
        COMMENT "Compiling SYCL executable vecadd-sycl"
        COMMAND_EXPAND_LISTS
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vecadd-sycl DESTINATION "."
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    add_custom_target(
        vecadd-sycl-multi ALL
        ${CSCC_COMMAND}
        --targets=all
        ${debug_opts}
        -O3
        -o vecadd-sycl-multi
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/sycl-multi.cpp
        DEPENDS main.cpp sycl-multi.cpp
        COMMENT "Compiling SYCL executable vecadd-sycl-multi"
        COMMAND_EXPAND_LISTS
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vecadd-sycl-multi DESTINATION "."
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
else()
    add_executable(vecadd-cxx main.cpp cxx.cpp)
    install(TARGETS vecadd-cxx DESTINATION ".")
endif()
