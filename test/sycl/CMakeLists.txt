set_directory_properties(PROPERTIES COMPILE_OPTIONS "" LINK_OPTIONS "")

set(
    tests

    buffer
    id
    platform
    range

    array
    # blas_dgemm
    capture
    capture2
    capture3
    capture4
    # cholesky
    copy
    copy2
    enum
    fill
    for
    function
    functor
    functor2
    functor3
    inherit
    item
    lambda
    local_accessor
    method
    nbody
    nd_item
    parallel_for2
    parallel_for3
    pointer
    queue_profiling
    reduction
    reference
    single_task
    sqrt
    struct
    struct2
    struct3
    struct4
    tag
    template
    vec
    vecadd
)

set(common_env OMP_NUM_THREADS=4)

set(rts_list CPU)
set(CPU_env CHARM_SYCL_RTS=CPU)

if(HAVE_NVIDIA_GPUS)
    message("-- Enabled: Tests with NVIDIA GPUs")

    list(APPEND rts_list CUDA)
    set(CUDA_env CHARM_SYCL_RTS=CUDA)
endif()

if(HAVE_AMD_GPUS)
    message("-- Enabled: Tests with AMD GPUs")

    list(APPEND rts_list HIP)
    set(HIP_env CHARM_SYCL_RTS=HIP)
endif()

if(HAVE_IRIS_FOR_TEST)
    message("-- Enabled: Tests with IRIS library")

    list(APPEND rts_list IRIS-openmp)
    set(IRIS-openmp_env CHARM_SYCL_RTS=IRIS IRIS_ARCHS=openmp)

    if(HAVE_NVIDIA_GPUS)
        list(APPEND rts_list IRIS-cuda)
        set(IRIS-cuda_env CHARM_SYCL_RTS=IRIS IRIS_ARCHS=cuda)
    endif()

    if(HAVE_AMD_GPUS)
        list(APPEND rts_list IRIS-hip)
        set(IRIS-hip_env CHARM_SYCL_RTS=IRIS IRIS_ARCHS=hip)
    endif()
endif()

set(targets)

foreach(test ${tests})
    set(exe test_${test})

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${exe}
        COMMAND
        "$<TARGET_FILE:cscc>"
        -save-kernels
        -save-xmls
        ${stop_at}
        -o ${exe}
        -g
        -MD
        -MF ${CMAKE_CURRENT_BINARY_DIR}/${test}.d
        -MT ${CMAKE_CURRENT_BINARY_DIR}/${test}
        -I${PROJECT_SOURCE_DIR}/vendor/ut/include
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

    add_custom_target(
        test-${test}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${exe}
    )

    list(APPEND targets test-${test})
endforeach()

list(APPEND TEST_DEPENDS ${targets})
set(TEST_DEPENDS ${TEST_DEPENDS} PARENT_SCOPE)

set(
    TEST_QUICK_DEPENDS

    test-local_accessor
    test-nd_item
    test-parallel_for2
    test-parallel_for3
    test-single_task
    PARENT_SCOPE
)

add_custom_target(
    check-sycl
    DEPENDS ${targets}
)
