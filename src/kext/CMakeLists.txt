set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
enable_asan_if_debug()

check_cxx_compiler_flag("-Wno-unused-parameter" CXX_HAVE_WNO_UNUSED_PARAMETER)

if(CXX_HAVE_WNO_UNUSED_PARAMETER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
endif()

check_cxx_compiler_flag("-Wno-deprecated-enum-enum-conversion" HAVE_WNO_DEPRECATED_ENUM_ENUM_COVERSION)

if(HAVE_WNO_DEPRECATED_ENUM_ENUM_COVERSION)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-enum-enum-conversion")
endif()

check_cxx_compiler_flag("-Wno-deprecated-anon-enum-enum-conversion" HAVE_WNO_DEPRECATED_ANON_ENUM_ENUM_CONVERSION)

if(HAVE_WNO_DEPRECATED_ANON_ENUM_ENUM_CONVERSION)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-anon-enum-enum-conversion")
endif()

add_executable(
    charm-kext

    charm-kext.cpp
    decl_visitor.cpp
    error_trace.cpp
    expr_visitor.cpp
    function_builder.cpp
    stmt_visitor.cpp
    struct_builder.cpp
    transform_info.cpp
    transform.cpp
    utils.cpp
)
target_include_directories(charm-kext PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(charm-kext PRIVATE sycl-headers clsy xcml utils fmt::fmt pugixml::pugixml)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(charm-kext PRIVATE -fno-rtti)
endif()

install(TARGETS charm-kext)
