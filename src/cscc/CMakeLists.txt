override_linker_command()

set(
    definitions
    -DCMAKE_C_COMPILER="${CMAKE_C_COMPILER}"
    -DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
    -DBOOST_ENABLE_ASSERT_HANDLER=1
)

if(APPLE)
    list(APPEND definitions -DCSCC_APPLE)
endif()

if(CUDAToolkit_FOUND)
    list(APPEND
        definitions
        -DCUDA_LIBRARY="${CUDA_cuda_driver_LIBRARY}"
        -DCMAKE_CUDA_COMPILER="${CUDAToolkit_NVCC_EXECUTABLE}"
        -DCUDA_cublas_LIBRARY="${CUDA_cublas_LIBRARY}"
        -DCUDA_cusolver_LIBRARY="${CUDA_cusolver_LIBRARY}"
    )
endif()

if(hip_FOUND)
    list(APPEND
        definitions
        -DHIP_LIB_INSTALL_DIR="${HIP_LIB_INSTALL_DIR}"
        -DHIP_HIPCC_EXECUTABLE="${HIP_HIPCC_EXECUTABLE}"
    )
endif()

if(IRIS_LIBRARY)
    list(APPEND
        definitions
        -DIRIS_LIBRARY="${IRIS_LIBRARY}"
    )
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

if(CLANG_FORMAT)
    list(APPEND definitions -DCLANG_FORMAT_COMMAND="${CLANG_FORMAT}")
endif()

if(TARGET openmp_loader_obj)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/iris_omp_loader.S
        COMMAND "$<TARGET_FILE:chsy-bin2asm>" "$<TARGET_OBJECTS:openmp_loader_obj>" iris_omp_loader ${CMAKE_CURRENT_BINARY_DIR}/iris_omp_loader.S
        DEPENDS "$<TARGET_FILE:chsy-bin2asm>" "$<TARGET_OBJECTS:openmp_loader_obj>"
    )
    list(APPEND clang_sources ${CMAKE_CURRENT_BINARY_DIR}/iris_omp_loader.S)
endif()

find_package(OpenMP)

if(OpenMP_FOUND)
    list(APPEND definitions -DOpenMP_FOUND -DOpenMP_C_FLAGS="${OpenMP_C_FLAGS}" -DOpenMP_C_LIBARIES="${OpenMP_C_LIBARIES}")
endif()

add_executable(fix EXCLUDE_FROM_ALL fix.cpp)
target_link_libraries(fix PRIVATE fmt::fmt Threads::Threads dl)

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

if(CSCC_PORTABLE_MODE)
    if(NOT LLVM_VERSION_MAJOR EQUAL 17)
        message(FATAL_ERROR "LLVM ${LLVM_PACKAGE_VERSION} is not supported. CSCC_PORTABLE_MODE requires LLVM 17.")
    endif()

    set(LLVM_DIR ${PROJECT_SOURCE_DIR}/vendor/llvm-project-17.0.4)

    list(
        APPEND
        clang_sources

        vfs.cpp
        ${LLVM_DIR}/clang/tools/driver/cc1_main.cpp
        ${LLVM_DIR}/clang/tools/driver/cc1as_main.cpp
        ${LLVM_DIR}/clang/tools/driver/cc1gen_reproducer_main.cpp
        ${LLVM_DIR}/clang/tools/driver/driver.cpp
        ${LLVM_DIR}/lld/tools/lld/lld.cpp
    )

    set(
        headers

        charm/sycl.hpp
        charm/sycl/access_mode.hpp
        charm/sycl/accessor.hpp
        charm/sycl/aspect.hpp
        charm/sycl/backend.hpp
        charm/sycl/buffer.hpp
        charm/sycl/buffer.ipp
        charm/sycl/context.hpp
        charm/sycl/context.ipp
        charm/sycl/device_accessor.hpp
        charm/sycl/device_accessor.ipp
        charm/sycl/device_type.hpp
        charm/sycl/device.hpp
        charm/sycl/device.ipp
        charm/sycl/event.hpp
        charm/sycl/event.ipp
        charm/sycl/exception.hpp
        charm/sycl/fnv1a.hpp
        charm/sycl/fwd.hpp
        charm/sycl/group.hpp
        charm/sycl/group.ipp
        charm/sycl/handler.hpp
        charm/sycl/handler.ipp
        charm/sycl/host_accessor.hpp
        charm/sycl/host_accessor.ipp
        charm/sycl/local_accessor.hpp
        charm/sycl/local_accessor.ipp
        charm/sycl/math_impl2.ipp
        charm/sycl/nd_item.hpp
        charm/sycl/nd_range.hpp
        charm/sycl/platform.hpp
        charm/sycl/platform.ipp
        charm/sycl/property.hpp
        charm/sycl/queue.hpp
        charm/sycl/queue.ipp
        charm/sycl/reduction.hpp
        charm/sycl/reduction.ipp
        charm/sycl/runtime/accessor.hpp
        charm/sycl/runtime/allocator.hpp
        charm/sycl/runtime/blas.hpp
        charm/sycl/runtime/buffer.hpp
        charm/sycl/runtime/context.hpp
        charm/sycl/runtime/device.hpp
        charm/sycl/runtime/event.hpp
        charm/sycl/runtime/fwd.hpp
        charm/sycl/runtime/handler.hpp
        charm/sycl/runtime/host_accessor_impl.hpp
        charm/sycl/runtime/intrusive_ptr.hpp
        charm/sycl/runtime/local_accessor.hpp
        charm/sycl/runtime/platform.hpp
        charm/sycl/runtime/property.hpp
        charm/sycl/runtime/queue.hpp
        charm/sycl/selector.hpp
        charm/sycl/selector.ipp
        charm/sycl/utils.hpp
        charm/sycl/utils.ipp
        charm/sycl/vec.hpp
        charm/sycl/vec.ipp
        CL/sycl.hpp
        sycl/sycl.hpp
    )

    set(
        generated_headers

        # from ${PROJECT_SOURCE_DIR}/generated/include
        charm/sycl/id_1.hpp
        charm/sycl/id_1.ipp
        charm/sycl/id_2.hpp
        charm/sycl/id_2.ipp
        charm/sycl/id_3.hpp
        charm/sycl/id_3.ipp
        charm/sycl/range_1.hpp
        charm/sycl/range_1.ipp
        charm/sycl/range_2.hpp
        charm/sycl/range_2.ipp
        charm/sycl/range_3.hpp
        charm/sycl/range_3.ipp

        # from ${PROJECT_BINARY_DIR}/include
        charm/sycl/config.hpp
        charm/sycl/device_info.hpp
        charm/sycl/device_info.ipp
        charm/sycl/item.hpp
        charm/sycl/item.ipp
        charm/sycl/math_impl.hpp
        charm/sycl/math.ipp
        charm/sycl/platform_info.hpp
        charm/sycl/platform_info.ipp
        charm/sycl/runtime/blas1.hpp
        charm/sycl/runtime/blas2.hpp
        charm/sycl/runtime/blas3.hpp
        charm/sycl/runtime/lapack.hpp
        charm/sycl/vec_fwd.hpp
        charm/sycl/vec_ops.hpp
        charm/sycl/vec_swizzle.hpp
    )

    set(
        clang_headers

        ${LLVM_DIR}/clang/lib/Headers/__stddef_max_align_t.h
        ${LLVM_DIR}/clang/lib/Headers/builtins.h
        ${LLVM_DIR}/clang/lib/Headers/float.h
        ${LLVM_DIR}/clang/lib/Headers/inttypes.h
        ${LLVM_DIR}/clang/lib/Headers/iso646.h
        ${LLVM_DIR}/clang/lib/Headers/limits.h
        ${LLVM_DIR}/clang/lib/Headers/stdalign.h
        ${LLVM_DIR}/clang/lib/Headers/stdarg.h
        ${LLVM_DIR}/clang/lib/Headers/stdatomic.h
        ${LLVM_DIR}/clang/lib/Headers/stdbool.h
        ${LLVM_DIR}/clang/lib/Headers/stddef.h
        ${LLVM_DIR}/clang/lib/Headers/stdint.h
        ${LLVM_DIR}/clang/lib/Headers/stdnoreturn.h
        ${LLVM_DIR}/clang/lib/Headers/tgmath.h
        ${LLVM_DIR}/clang/lib/Headers/unwind.h
        ${LLVM_DIR}/clang/lib/Headers/varargs.h
    )

    foreach(file ${headers})
        set(input ${PROJECT_SOURCE_DIR}/include/${file})
        STRING(REPLACE / _ prefix ${file})
        STRING(REPLACE . _ prefix ${prefix})
        set(output ${CMAKE_CURRENT_BINARY_DIR}/${file}.S)
        cmake_path(GET output PARENT_PATH dir)
        file(MAKE_DIRECTORY ${dir})

        add_custom_command(
            OUTPUT ${output}
            COMMAND "$<TARGET_FILE:chsy-bin2asm>" ${input} ${prefix} ${output}
            DEPENDS "$<TARGET_FILE:chsy-bin2asm>" ${input}
        )
        list(APPEND clang_sources ${output})
    endforeach()

    foreach(file ${generated_headers})
        set(input ${PROJECT_SOURCE_DIR}/generated/include/${file})

        if(NOT EXISTS ${input})
            set(input ${PROJECT_BINARY_DIR}/include/${file})
        endif()

        STRING(REPLACE / _ prefix ${file})
        STRING(REPLACE . _ prefix ${prefix})
        set(output ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.S)
        cmake_path(GET output PARENT_PATH dir)
        file(MAKE_DIRECTORY ${dir})

        add_custom_command(
            OUTPUT ${output}
            COMMAND "$<TARGET_FILE:chsy-bin2asm>" ${input} ${prefix} ${output}
            DEPENDS "$<TARGET_FILE:chsy-bin2asm>" ${input}
        )
        list(APPEND clang_sources ${output})
    endforeach()

    foreach(file ${clang_headers})
        set(input ${file})
        cmake_path(GET input FILENAME file)
        STRING(REPLACE / _ prefix ${file})
        STRING(REPLACE . _ prefix ${prefix})
        STRING(REPLACE - _ prefix ${prefix})
        set(output ${CMAKE_CURRENT_BINARY_DIR}/${prefix}.S)
        cmake_path(GET output PARENT_PATH dir)
        file(MAKE_DIRECTORY ${dir})

        add_custom_command(
            OUTPUT ${output}
            COMMAND "$<TARGET_FILE:chsy-bin2asm>" ${input} ${prefix} ${output}
            DEPENDS "$<TARGET_FILE:chsy-bin2asm>" ${input}
        )
        list(APPEND clang_sources ${output})
    endforeach()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libsycl.a.S
        COMMAND "$<TARGET_FILE:chsy-bin2asm>" "$<TARGET_FILE:sycl>" libsycl_a ${CMAKE_CURRENT_BINARY_DIR}/libsycl.a.S
        DEPENDS "$<TARGET_FILE:chsy-bin2asm>" "$<TARGET_FILE:sycl>"
    )
    list(APPEND clang_sources ${CMAKE_CURRENT_BINARY_DIR}/libsycl.a.S)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libc++.a.S
        COMMAND "$<TARGET_FILE:chsy-bin2asm>" ${LLVM_LIBRARY_DIRS}/${LLVM_TARGET_TRIPLE}/libc++.a libcxx_a ${CMAKE_CURRENT_BINARY_DIR}/libc++.a.S
        DEPENDS "$<TARGET_FILE:chsy-bin2asm>" ${LLVM_LIBRARY_DIRS}/${LLVM_TARGET_TRIPLE}/libc++.a
    )
    list(APPEND clang_sources ${CMAKE_CURRENT_BINARY_DIR}/libc++.a.S)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libc++abi.a.S
        COMMAND "$<TARGET_FILE:chsy-bin2asm>" ${LLVM_LIBRARY_DIRS}/${LLVM_TARGET_TRIPLE}/libc++abi.a libcxxabi_a ${CMAKE_CURRENT_BINARY_DIR}/libc++abi.a.S
        DEPENDS "$<TARGET_FILE:chsy-bin2asm>" ${LLVM_LIBRARY_DIRS}/${LLVM_TARGET_TRIPLE}/libc++abi.a
    )
    list(APPEND clang_sources ${CMAKE_CURRENT_BINARY_DIR}/libc++abi.a.S)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vfs.ipp
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vfs.py ${headers} ${generated_headers} --clang ${clang_headers}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vfs.py
    )
    list(APPEND clang_sources ${CMAKE_CURRENT_BINARY_DIR}/vfs.ipp)

    find_package(LLD REQUIRED)

    set(
        clang_libs

        clangFrontendTool
        lldCOFF
        lldELF
        lldMachO
        lldMinGW
        lldWasm
        LLVM${LLVM_NATIVE_ARCH}AsmParser
        LLVM${LLVM_NATIVE_ARCH}CodeGen
    )

    set_source_files_properties(${clang_sources} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
endif()

set(
    sources

    cc_kernel.cpp
    compile_host.cpp
    cscc_main.cpp
    cscc.cpp
    hipcc.cpp
    link_host.cpp
    make_binary_loader.cpp
    marked.cpp
    nvcc.cpp
    packed.cpp
    parse_args.cpp
    save_temps.cpp
    subprocess.cpp
    tools.cpp
)

set(
    libs

    cscc-bin2asm
    cscc-cback
    cscc-get
    cscc-kext
    cscc-lower
    fmt::fmt
    sycl-headers
    utils-errors
    utils-hash
    utils-io
    utils-target
    xcml
    dl
    Threads::Threads
    ${clang_libs}
)

list(
    APPEND
    definitions
    "$<$<BOOL:${CHARM_SYCL_ENABLE_OPENBLAS}>:CHARM_SYCL_ENABLE_OPENBLAS>"
)

if(CHARM_SYCL_ENABLE_OPENBLAS)
    list(
        APPEND
        definitions
        OPENBLAS_LIBRARY="$<TARGET_LINKER_FILE:${OPENBLAS_TARGET}>"
        LAPACKE_LIBRARY="$<TARGET_LINKER_FILE:${LAPACKE_TARGET}>"
    )
endif()

add_library(cscc-common OBJECT ${sources} ${clang_sources})
target_compile_definitions(cscc-common PUBLIC ${definitions})
target_link_libraries(cscc-common PUBLIC ${libs})
target_include_directories(cscc-common PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${LLVM_INCLUDE_DIRS})

if(CSCC_PORTABLE_MODE)
    target_compile_definitions(cscc-common PUBLIC -DCSCC_PORTABLE_MODE)
endif()

add_executable(cscc config.cpp ${config_h})
target_link_libraries(cscc PRIVATE cscc-common ${libs})
target_include_directories(cscc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(
    cscc PRIVATE
    -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    -DPROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}"
    "$<$<BOOL:${ENABLE_BLAS}>:CHARM_SYCL_ENABLE_BLAS>"
    "$<$<BOOL:${ENABLE_LAPACK}>:CHARM_SYCL_ENABLE_LAPACK>"
)

if(NOT CSCC_PORTABLE_MODE)
    add_executable(cscc-install config.cpp ${config_h})
    target_link_libraries(cscc-install PRIVATE cscc-common ${libs})
    target_include_directories(cscc-install PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_definitions(
        cscc-install PRIVATE
        -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
    )

    install(PROGRAMS "$<TARGET_FILE:cscc-install>" RENAME cscc DESTINATION "bin")
else()
    install(PROGRAMS "$<TARGET_FILE:cscc>" DESTINATION "bin")
endif()
