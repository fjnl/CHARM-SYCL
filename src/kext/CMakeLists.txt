add_library(
    cscc-kext
    OBJECT
    ast_visitor.cpp
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
    visitor_base.cpp
)
target_include_directories(cscc-kext PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${LLVM_INCLUDE_DIRS}")
target_link_libraries(cscc-kext PRIVATE sycl-headers xcml utils fmt::fmt pugixml::pugixml)

if(NOT CSCC_PORTABLE_MODE AND CHARM_SYCL_USE_CLANG_DYLIB)
    if(NOT TARGET LLVM)
        message(FATAL_ERROR "cannot find LLVM dylib")
    endif()

    if(NOT CLANG_LINK_CLANG_DYLIB OR NOT TARGET clang-cpp)
        message(FATAL_ERROR "cannot find clang dylib")
    endif()

    target_link_libraries(cscc-kext PRIVATE LLVM clang-cpp)
else()
    target_link_libraries(cscc-kext PRIVATE clangTooling)
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(cscc-kext PRIVATE -fno-rtti)
endif()
