cmake_minimum_required(VERSION 3.20)

project(charm-sycl LANGUAGES C CXX ASM VERSION 0.0.1)

include(CMakeDependentOption)

option(CSCC_PORTABLE_MODE OFF "Experimental: Enable the compiler portable mode")

option(CHARM_SYCL_USE_WERROR "Use -Werror if the C++ compiler supports. (default: NO)" NO)
set(CHARM_SYCL_ENABLE_LOGGING "" CACHE STRING "Enable logging in the runtime. (default: YES if CMAKE_BUILD_TYPE is Debug, otherwise NO)")

cmake_dependent_option(USE_IRIS "Use IRIS library (default: NO)" NO "NOT CSCC_PORTABLE_MODE" OFF)
cmake_dependent_option(IRIS_DIR "Specified directory for IRIS library (default: auto detect)" OFF "NOT CSCC_PORTABLE_MODE" OFF)
cmake_dependent_option(CHARM_SYCL_IRIS_IS_REQUIRED "Raise error if IRIS is not found. (default: NO)" NO "NOT CSCC_PORTABLE_MODE" OFF)

# option(CHARM_SYCL_LINK_STATIC_CUDART "Link static CUDA runtime library (default: YES)" YES)
cmake_dependent_option(CHARM_SYCL_CUDA_IS_REQUIRED "Raise error if CUDA is not found. (default: NO)" NO "NOT CSCC_PORTABLE_MODE" OFF)
option(CHARM_SYCL_USE_CLANG_DYLIB "Link clang dylib instead of individual static libraries (default: YES)" YES)
set(CHARM_SYCL_RUNTIME_STDLIB "" CACHE STRING "set value of the -stdlib= option for the SYCL runtime library. (default: the option is not used)")

cmake_dependent_option(CHARM_SYCL_HIP_IS_REQUIRED "Raise error if HIP is not found. (default: NO)" NO "NOT CSCC_PORTABLE_MODE" OFF)

set(CSCC_USE_LINKER "" CACHE STRING "Override the linker used by cscc")
option(CSCC_USE_LIBCXX OFF "Force cscc use libc++")
option(CSCC_USE_LIBCXX_STATIC OFF "cscc links libc++ statically")
set(CSCC_LIBCXXABI_PATH "" CACHE STRING "Force cscc use this path as the libc++abi library")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

message("-- CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}.")

if(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    message("-- CMAKE_POSITION_INDEPENDENT_CODE is not defined; set it to YES")
    set(CMAKE_POSITION_INDEPENDENT_CODE YES)
endif()

if(NOT DEFINED CMAKE_OPTIMIZE_DEPENDENCIES)
    message("-- CMAKE_OPTIMIZE_DEPENDENCIES is not defined; set it to YES")
    set(CMAKE_OPTIMIZE_DEPENDENCIES YES)
endif()

if(NOT DEFINED CHARM_SYCL_ENABLE_LOGGING)
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        set(CHARM_SYCL_ENABLE_LOGGING YES)
    endif()
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
    if(NOT CMAKE_COLOR_DIAGNOSTICS)
        set(CMAKE_COLOR_DIAGNOSTICS "ON")
    endif()
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
find_package(Threads)
find_package(Python COMPONENTS Interpreter REQUIRED)

if(NOT CMAKE_C_COMPILER_ID STREQUAL Clang)
    message(FATAL_ERROR "The C compiler is not clang: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    message(FATAL_ERROR "The C++ compiler is not clang: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
endif()

if(LLVM_VERSION_MAJOR LESS 13)
    message(FATAL_ERROR "requires LLVM >= 13 (${LLVM_PACKAGE_VERSION})")
endif()

if(NOT CMAKE_C_COMPILER_VERSION MATCHES "^${LLVM_VERSION_MAJOR}\.")
    message(FATAL_ERROR "The clang compiler version is not matched with the LLVM vesrion: clang=${CMAKE_C_COMPILER_VERSION}, llvm=${LLVM_PACKAGE_VERSION}")
endif()

if(NOT CMAKE_CXX_COMPILER_VERSION MATCHES "^${LLVM_VERSION_MAJOR}\.")
    message(FATAL_ERROR "The clang++ compiler version is not matched with the LLVM vesrion: clang++=${CMAKE_CXX_COMPILER_VERSION}, llvm=${LLVM_PACKAGE_VERSION}")
endif()

if(NOT CSCC_PORTABLE_MODE)
    if(CHARM_SYCL_CUDA_IS_REQUIRED)
        find_package(CUDAToolkit REQUIRED)
    else()
        find_package(CUDAToolkit)
    endif()
endif()

if(NOT CSCC_PORTABLE_MODE)
    if(CHARM_SYCL_HIP_IS_REQUIRED)
        find_package(hip REQUIRED)
    else()
        find_package(hip QUIET)
    endif()
endif()

if(NOT DEFINED hip_FOUND)
    set(hip_FOUND NO)
endif()

set(CHARM_SYCL_ENABLE_BLAS 0)
set(CHARM_SYCL_ENABLE_CUDA 0)
set(CHARM_SYCL_ENABLE_HIP 0)
set(CHARM_SYCL_ENABLE_LAPACK 0)
set(CHARM_SYCL_ENABLE_OPENBLAS 0)

if(CUDAToolkit_FOUND)
    set(CHARM_SYCL_ENABLE_CUDA 1)
endif()

if(hip_FOUND)
    set(CHARM_SYCL_ENABLE_HIP 1)
endif()

if(CSCC_PORTABLE_MODE)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>")
    add_link_options(-stdlib=libc++ -static-libstdc++)
endif()

function(override_linker_command)
    if(CSCC_PORTABLE_MODE)
        set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} ${LLVM_LIBRARY_DIRS}/${LLVM_TARGET_TRIPLE}/libc++.a ${LLVM_LIBRARY_DIRS}/${LLVM_TARGET_TRIPLE}/libc++abi.a" PARENT_SCOPE)
    endif()
endfunction()

add_subdirectory(vendor/boost/array)
add_subdirectory(vendor/boost/assert)
add_subdirectory(vendor/boost/config)
add_subdirectory(vendor/boost/container_hash)
add_subdirectory(vendor/boost/context)
add_subdirectory(vendor/boost/core)
add_subdirectory(vendor/boost/describe)
add_subdirectory(vendor/boost/integer)
add_subdirectory(vendor/boost/leaf)
add_subdirectory(vendor/boost/move)
add_subdirectory(vendor/boost/mp11)
add_subdirectory(vendor/boost/pool)
add_subdirectory(vendor/boost/predef)
add_subdirectory(vendor/boost/smart_ptr)
add_subdirectory(vendor/boost/stacktrace)
add_subdirectory(vendor/boost/static_assert)
add_subdirectory(vendor/boost/throw_exception)
add_subdirectory(vendor/boost/type_traits)
add_subdirectory(vendor/boost/winapi)
add_subdirectory(vendor/cxxopts)
add_subdirectory(vendor/fmtlib)
add_subdirectory(vendor/pugixml)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckCXXCompilerFlag)
include(CheckIncludeFile)

add_compile_options(-Wall -Wextra)

if(CHARM_SYCL_ENABLE_WERROR)
    add_compile_options(-Werror)
endif()

find_program(CLANG_FORMAT NAMES clang-format clang-format-${LLVM_VERSION_MAJOR} PATHS ${LLVM_TOOLS_BINARY_DIR})

if(CLANG_FORMAT)
    if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
        file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format ${CMAKE_CURRENT_BINARY_DIR}/.clang-format)
    endif()
endif()

if(USE_IRIS OR IRIS_DIR)
    if(IRIS_DIR)
        message("-- IRIS_DIR: ${IRIS_DIR}")
    endif()

    find_library(IRIS_LIBRARY NAMES iris PATHS ${IRIS_DIR}/lib)
endif()

if(IRIS_LIBRARY)
    set(USE_IRIS YES)

    message("-- IRIS library found: ${IRIS_LIBRARY}")
    file(REAL_PATH "${IRIS_LIBRARY}/../.." IRIS_ROOT)
    set(IRIS_INCLUDE_DIR "${IRIS_ROOT}/include")

    message("-- IRIS_ROOT: ${IRIS_ROOT}")
    message("-- IRIS_INCLUDE_DIR: ${IRIS_INCLUDE_DIR}")

    set(CMAKE_REQUIRED_INCLUDES "${IRIS_INCLUDE_DIR}")
    CHECK_INCLUDE_FILE("iris/iris.h" Iris_h)

    if(Iris_h)
        message("-- iris/iris.h found")
    else()
        message(FATAL_ERROR "-- iris/iris.h: not found")
    endif()

    add_library(Iris INTERFACE)
    target_link_libraries(Iris INTERFACE ${IRIS_LIBRARY})
    target_include_directories(Iris INTERFACE ${IRIS_INCLUDE_DIR})
else()
    set(USE_IRIS NO)

    if(CHARM_SYCL_IRIS_IS_REQUIRED)
        message(FATAL_ERROR "-- IRIS Library not found")
    else()
        message("-- IRIS Library not found")
    endif()
endif()

set(CHARM_SYCL_VERSION "${CMAKE_PROJECT_VERSION}")
configure_file(include/charm/sycl/config.hpp.in include/charm/sycl/config.hpp)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/charm/sycl/config.hpp DESTINATION include/charm/sycl)

add_subdirectory(internal)
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(bench)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR AND BUILD_TESTING)
    message("-- ${PROJECT_NAME}: tests are enabled")

    include(CTest)
    enable_testing()

    # add_subdirectory(vendor/Catch2)
    add_subdirectory(test)
endif()
