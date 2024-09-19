override_linker_command()
add_compile_options(-fno-rtti)

add_executable(interface_gen EXCLUDE_FROM_ALL interface_gen.cpp)
target_include_directories(interface_gen PRIVATE "${LLVM_INCLUDE_DIRS}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(interface_gen PRIVATE pugixml::pugixml)

if(NOT CSCC_PORTABLE_MODE AND CHARM_SYCL_USE_CLANG_DYLIB)
    if(NOT TARGET LLVM)
        message(FATAL_ERROR "cannot find LLVM dylib")
    endif()

    if(NOT CLANG_LINK_CLANG_DYLIB OR NOT TARGET clang-cpp)
        message(FATAL_ERROR "cannot find clang dylib")
    endif()

    target_link_libraries(interface_gen PRIVATE LLVM clang-cpp)
else()
    target_link_libraries(interface_gen PRIVATE clangTooling)
endif()
