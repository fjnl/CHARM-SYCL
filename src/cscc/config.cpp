#include "config.h"
#include <filesystem>

std::filesystem::path cscc_tool_path([[maybe_unused]] char const* dir, char const* name) {
#ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_BINARY_DIR) / "src" / dir / name;
#else
    return std::filesystem::path(CMAKE_INSTALL_PREFIX) / "bin" / name;
#endif
}

std::filesystem::path cscc_include_path_bin() {
#ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_BINARY_DIR) / "include";
#else
    return std::filesystem::path(CMAKE_INSTALL_PREFIX) / "include";
#endif
}

std::filesystem::path cscc_include_path_src() {
#ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_SOURCE_DIR) / "include";
#else
    return "";
#endif
}

std::filesystem::path cscc_sycl_runtime_library() {
#ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_BINARY_DIR) / RUNTIME_LIBRARY;
#else
    auto const filename = std::filesystem::path(RUNTIME_LIBRARY).filename();
    return std::filesystem::path(CMAKE_INSTALL_PREFIX) / "lib" / filename;
#endif
}

#ifdef USE_IRIS
std::filesystem::path cscc_iris_omp_loader() {
#    ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_BINARY_DIR) / IRIS_OPENMP_LOADER_NAME;
#    else
    auto const filename = std::filesystem::path(IRIS_OPENMP_LOADER_NAME).filename();
    return cscc_sycl_runtime_library().parent_path() / filename;
#    endif
}
#endif
