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

if(CXX_HAVE_WNO_UNUSED_PARAMETER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
endif()

add_library(clsy STATIC visitor.cpp)
target_include_directories(clsy PUBLIC "${LLVM_INCLUDE_DIRS}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(clsy PRIVATE fmt::fmt)

if(CHARM_SYCL_USE_CLANG_DYLIB)
    if(NOT TARGET LLVM)
        message(FATAL_ERROR "cannot find LLVM dylib")
    endif()

    if(NOT CLANG_LINK_CLANG_DYLIB OR NOT TARGET clang-cpp)
        message(FATAL_ERROR "cannot find clang dylib")
    endif()

    target_link_libraries(clsy PRIVATE LLVM clang-cpp)
else()
    target_link_libraries(clsy PRIVATE clangTooling)
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(clsy PRIVATE -fno-rtti)
endif()

if(CHARM_SYCL_ENABLE_ASAN AND APPLE)
    target_compile_definitions(clsy PRIVATE -DCHARM_SYCL_ASAN_WORKAROUND)
endif()
