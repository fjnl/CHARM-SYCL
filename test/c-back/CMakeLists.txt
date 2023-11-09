function(add name)
    set(t_setup "C-BACK: setup ${name}")
    set(t_run "C-BACK: run   ${name}")

    set(test_cpp "${name}.cpp")
    set(xml "${CMAKE_CURRENT_SOURCE_DIR}/${name}.xml")
    set(c "${CMAKE_CURRENT_BINARY_DIR}/${name}.c")

    add_executable(${name} ${test_cpp})

    add_test(NAME ${t_setup} COMMAND "$<TARGET_FILE:chsy-c-back>" -o ${c} --target cpu-c ${xml})
    set_tests_properties(${t_setup} PROPERTIES FIXTURES_SETUP ${t_setup})

    add_test(NAME ${t_run} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${name} ${c})
    set_tests_properties(${t_run} PROPERTIES FIXTURES_REQUIRED ${t_setup})

    list(APPEND TEST_DEPENDS "c-back/${name}")
endfunction()

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CHARM_SYCL_ENABLE_ASAN)
    set(asan_opts "-fsanitize=address")
endif()

add(cback_00)
add(cback_01)
add(cback_02)
add(cback_03)
add(cback_04)
add(cback_05)
add(cback_06)
add(cback_07)
set(TEST_DEPENDS ${TEST_DEPENDS} PARENT_SCOPE)
