set(
    tests

    buffer
    id
    platform
    range

    array
    capture
    capture2
    capture3
    capture4
    copy
    copy2
    div
    enum
    fill
    for
    function
    functor
    functor2
    functor3
    inherit
    item
    local_accessor
    method
    minus
    mul
    nbody
    nd_item
    parallel_for
    parallel_for2
    parallel_for3
    plus
    pointer
    queue_profiling
    reference
    single_task
    sqrt
    struct
    struct2
    struct3
    struct4
    struct5
    tag
    template
    vec
    vecadd
)

set(common_env OMP_NUM_THREADS=4)
set(asan_opts "allow_user_poisoning=false:detect_container_overflow=0")
set(asan_cuda_opts "protect_shadow_gap=false")
set(asan_iris_opts "detect_leaks=0")

set(rts_list CPU)
set(CPU_env CHARM_SYCL_RTS=CPU ASAN_OPTIONS=${asan_opts})

if(HAVE_NVIDIA_GPUS)
    list(APPEND rts_list CUDA)
    set(CUDA_env CHARM_SYCL_RTS=CUDA ASAN_OPTIONS=${asan_opts}:${asan_cuda_opts})
endif()

if(HAVE_AMD_GPUS)
    list(APPEND rts_list HIP)
    set(HIP_env CHARM_SYCL_RTS=HIP ASAN_OPTIONS=${asan_opts}:${asan_cuda_opts})
endif()

if(USE_IRIS)
    list(APPEND rts_list IRIS-openmp)
    set(IRIS-openmp_env CHARM_SYCL_RTS=IRIS IRIS_ARCHS=openmp ASAN_OPTIONS=${asan_opts}:${asan_iris_opts})

    if(HAVE_NVIDIA_GPUS)
        list(APPEND rts_list IRIS-cuda)
        set(IRIS-cuda_env CHARM_SYCL_RTS=IRIS IRIS_ARCHS=cuda ASAN_OPTIONS=${asan_opts}:${asan_iris_opts}:${asan_cuda_opts})
    endif()

    if(HAVE_AMD_GPUS)
        list(APPEND rts_list IRIS-hip)
        set(IRIS-hip_env CHARM_SYCL_RTS=IRIS IRIS_ARCHS=hip ASAN_OPTIONS=${asan_opts}:${asan_iris_opts}:${asan_cuda_opts})
    endif()
endif()

set(targets)

foreach(test ${tests})
    set(exe test_${test})

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${exe}
        COMMAND
        "$<TARGET_FILE:cscc>"
        --targets=all
        -save-kernels
        -save-xmls
        ${stop_at}
        -o ${exe}
        -g
        -MD
        -MF ${CMAKE_CURRENT_BINARY_DIR}/${test}.d
        -MT ${CMAKE_CURRENT_BINARY_DIR}/${test}
        "-I$<JOIN:$<TARGET_PROPERTY:Catch2,INCLUDE_DIRECTORIES>,;-I>"
        "$<TARGET_LINKER_FILE:Catch2>"
        ${CMAKE_CURRENT_SOURCE_DIR}/${test}.cpp
        DEPENDS
        "$<TARGET_FILE:cscc>"
        "$<TARGET_FILE:sycl>"
        ${CHARM_SCYL_COMMANDS}
        DEPFILE
        ${exe}.d
        COMMENT
        "Compiling SYCL executable ${exe}"
        COMMAND_EXPAND_LISTS
    )

    foreach(rts ${rts_list})
        set(name "${test}-${rts}")
        add_test(NAME ${name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${exe} --allow-running-no-tests)
        set_tests_properties(${name} PROPERTIES ENVIRONMENT "${common_env};${${rts}_env}")
    endforeach()

    list(APPEND targets "sycl/test_${test}")
endforeach()

list(APPEND TEST_DEPENDS ${targets})
set(TEST_DEPENDS ${TEST_DEPENDS} PARENT_SCOPE)

add_custom_target(
    check-sycl
    DEPENDS ${targets}
)
