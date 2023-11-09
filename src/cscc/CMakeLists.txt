set(
    definitions
    -DCMAKE_C_COMPILER="${CMAKE_C_COMPILER}"
    -DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
    -DCMAKE_AR="$<TARGET_FILE:llvm-ar>"
    -DOBJCOPY_COMMAND="$<TARGET_FILE:llvm-objcopy>"
    -DOBJDUMP_COMMAND="$<TARGET_FILE:llvm-objdump>"
    -DNM_COMMAND="$<TARGET_FILE:llvm-nm>"
)

if(APPLE)
    list(APPEND definitions -DCSCC_APPLE)
endif()

if(CUDAToolkit_FOUND)
    list(APPEND
        definitions
        -DCUDAToolkit
        -DCUDA_LIBRARY="$<TARGET_FILE:CUDA::cuda_driver>"
    )
endif()

if(hip_FOUND)
    message("-- HIP_LIB_INSTALL_DIR: ${HIP_LIB_INSTALL_DIR}")
    list(APPEND
        definitions
        -Dhip_FOUND
        -DHIP_LIB_INSTALL_DIR="${HIP_LIB_INSTALL_DIR}"
        -DHIP_HIPCC_EXECUTABLE="${HIP_HIPCC_EXECUTABLE}"
    )
endif()

if(CHARM_SYCL_ENABLE_ASAN)
    list(APPEND definitions -DCHARM_SYCL_ENABLE_ASAN)
endif()

get_target_property(type sycl TYPE)

if(type STREQUAL SHARED_LIBRARY)
    list(APPEND definitions -DCHARM_SYCL_IS_SHARED_LIBRARY)
endif()

if(DEFINED CSCC_USE_LINKER AND NOT CSCC_USE_LINKER STREQUAL "")
    list(APPEND definitions -DCSCC_USE_LINKER="${CSCC_USE_LINKER}")
endif()

if(DEFINED CSCC_USE_LIBCXX AND CSCC_USE_LIBCXX)
    list(APPEND definitions -DCSCC_USE_LIBCXX)
endif()

if(DEFINED CSCC_USE_LIBCXX_STATIC AND CSCC_USE_LIBCXX_STATIC)
    list(APPEND definitions -DCSCC_USE_LIBCXX_STATIC)
endif()

if(DEFINED CSCC_LIBCXXABI_PATH AND NOT CSCC_LIBCXXABI_PATH STREQUAL "")
    list(APPEND definitions -DCSCC_LIBCXXABI_PATH="${CSCC_LIBCXXABI_PATH}")
endif()

if(DEFINED CSCC_CUDA_HOST_COMPILER AND CSCC_CUDA_HOST_COMPILER)
    list(APPEND definitions -DCMAKE_CUDA_HOST_COMPILER="${CSCC_CUDA_HOST_COMPILER}")
endif()

list(APPEND
    definitions
    -DCMAKE_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
    -DCMAKE_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        list(APPEND definitions -DOBJCOPY_OUTPUT_FORMAT="elf64-x86-64")
    endif()
endif()

if(USE_IRIS)
    list(APPEND
        definitions
        -DUSE_IRIS
        -DIRIS_LIBRARY="${IRIS_LIBRARY}"
    )
endif()

find_program(CLANG_FORMAT NAMES clang-format clang-format-${LLVM_VERSION_MAJOR} PATHS ${LLVM_TOOLS_BINARY_DIR})

if(CLANG_FORMAT)
    list(APPEND definitions -DCLANG_FORMAT_COMMAND="${CLANG_FORMAT}")
endif()

if(TARGET openmp_loader_obj)
    set(opts IRIS_OPENMP_LOADER_NAME "$<TARGET_OBJECTS:openmp_loader_obj>")
endif()

find_package(OpenMP)

if(OpenMP_FOUND)
    list(APPEND definitions -DOpenMP_FOUND -DOpenMP_C_FLAGS="${OpenMP_C_FLAGS}" -DOpenMP_C_LIBARIES="${OpenMP_C_LIBARIES}")
endif()

add_executable(fix EXCLUDE_FROM_ALL fix.cpp)
target_link_libraries(fix PRIVATE fmt::fmt)

set(config_h ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_custom_command(
    OUTPUT
    ${config_h}
    COMMAND
    "$<TARGET_FILE:fix>"
    ${config_h}
    ${PROJECT_BINARY_DIR}
    RUNTIME_LIBRARY "$<TARGET_FILE:sycl>"
    ${opts}
    DEPENDS
    fix
    COMMAND_EXPAND_LISTS
)

add_library(cscc-common OBJECT cscc.cpp)
target_compile_definitions(cscc-common PUBLIC ${definitions})
target_link_libraries(cscc-common PUBLIC fmt::fmt utils-hash utils-target)

add_executable(cscc config.cpp ${config_h})
target_link_libraries(cscc PRIVATE cscc-common utils-target)
target_include_directories(cscc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(
    cscc PRIVATE
    -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    -DPROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}"
)

add_executable(cscc-install config.cpp ${config_h})
target_link_libraries(cscc-install PRIVATE cscc-common utils-target)
target_include_directories(cscc-install PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(
    cscc-install PRIVATE
    -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
)

install(PROGRAMS "$<TARGET_FILE:cscc-install>" RENAME cscc DESTINATION "bin")
