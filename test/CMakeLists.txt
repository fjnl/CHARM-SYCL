function(find_commands)
    set(commands)
    get_directory_property(subdirs DIRECTORY ../src SUBDIRECTORIES)

    foreach(dir ${subdirs})
        get_directory_property(targets DIRECTORY ${dir} BUILDSYSTEM_TARGETS)

        foreach(target ${targets})
            get_target_property(type ${target} TYPE)

            if(NOT type STREQUAL OBJECT_LIBRARY)
                list(APPEND commands ${target})
            endif()
        endforeach()
    endforeach()

    set(commands ${commands} PARENT_SCOPE)
endfunction()

find_commands()
set(libs sycl)

get_target_property(type sycl TYPE)

if(type STREQUAL SHARED_LIBRARY)
    set(CHARM_SYCL_IS_SHARED_LIBRARY YES)
else()
    set(CHARM_SYCL_IS_SHARED_LIBRARY NO)
endif()

foreach(cmd ${commands})
    list(APPEND command_files "$<TARGET_FILE:${cmd}>")
endforeach()

find_program(NVIDIA_SMI NAMES nvidia-smi)

if(NVIDIA_SMI)
    execute_process(COMMAND ${NVIDIA_SMI} -q OUTPUT_QUIET ERROR_QUIET RESULTS_VARIABLE result)

    if(result EQUAL 0)
        set(HAVE_NVIDIA_GPUS YES)
    endif()
endif()

find_program(ROCM_SMI NAMES rocm-smi)

if(ROCM_SMI)
    execute_process(
        COMMAND ${ROCM_SMI}
        OUTPUT_QUIET
        ERROR_VARIABLE err
        ERROR_STRIP_TRAILING_WHITESPACE
        RESULTS_VARIABLE result)

    if(result EQUAL 0 AND NOT err MATCHES "No AMD GPUs")
        set(HAVE_AMD_GPUS YES)
    endif()
endif()

if(TARGET Iris)
    set(HAVE_IRIS_FOR_TEST YES)
else()
    find_library(found NAMES iris HINTS ENV LD_LIBRARY_PATH)

    if(found)
        set(HAVE_IRIS_FOR_TEST YES)
    endif()
endif()

set(TEST_DEPENDS)
set(TEST_QUICK_DEPENDS)

add_subdirectory(c-back)
add_subdirectory(sycl)

add_custom_target(
    check
    DEPENDS check-full
)

add_custom_target(
    check-build
    DEPENDS ${TEST_DEPENDS}
)

add_custom_target(
    check-full
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --progress -j2
    DEPENDS ${TEST_DEPENDS}
    USES_TERMINAL
)

add_custom_target(
    check-quick
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --progress -j2 -R \"nd_item|parallel_for.*|single_task\"
    DEPENDS ${TEST_QUICK_DEPENDS}
    USES_TERMINAL
)
