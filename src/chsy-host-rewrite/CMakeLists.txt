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

set(target chsy-host-rewrite)
add_executable(${target} chsy-host-rewrite.cpp rewrite.cpp)
target_link_libraries(${target} PRIVATE fmt::fmt clsy)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(chsy-host-rewrite PRIVATE -fno-rtti)
endif()

install(TARGETS ${target})
