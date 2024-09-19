#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <boost/leaf.hpp>
#include <fmt/format.h>
#include <utils/target.hpp>

namespace detail {

struct strhash {
    using is_transparent = void;

    size_t operator()(char const* s) const {
        return (*this)(std::string_view(s));
    }

    size_t operator()(std::string const& s) const {
        return (*this)(std::string_view(s));
    }

    size_t operator()(std::string_view s) const {
        return std::hash<std::string_view>()(s);
    }
};

template <class V>
using str_map = std::unordered_map<std::string, V, strhash, std::equal_to<>>;

}  // namespace detail

#ifndef CMAKE_CUDA_COMPILER
#    define CMAKE_CUDA_COMPILER "nvcc"
#endif

#ifndef CMAKE_CUDA_HOST_COMPILER
#    define CMAKE_CUDA_HOST_COMPILER ""
#endif

#ifndef CLANG_FORMAT_COMMAND
#    define CLANG_FORMAT_COMMAND ""
#endif

#ifndef HIP_HIPCC_EXECUTABLE
#    define HIP_HIPCC_EXECUTABLE "hipcc"
#endif

#ifndef CSCC_PORTABLE_MODE
std::filesystem::path cscc_include_path_gen();
std::filesystem::path cscc_include_path_bin();
std::filesystem::path cscc_include_path_src();
std::filesystem::path cscc_sycl_runtime_library();
#endif

struct config_error {
    template <class... T>
    explicit config_error(fmt::format_string<T...> fmt, T&&... args)
        : msg(fmt::format(fmt, std::forward<T>(args)...)) {}

    std::string msg;

    friend std::ostream& operator<<(std::ostream& os, config_error const& err) {
        return os << err.msg;
    }
};

struct config {
    config();

    config(config const&) = delete;

    config(config&&) = delete;

    config& operator=(config const&) = delete;

    config& operator=(config&&) = delete;

    ~config();

    std::vector<std::string> inputs;
    std::vector<utils::target> targets;
    std::string output;
    bool verbose = false;
    bool show_version = false;
    bool save_temps = false;
    bool save_kernels = false;
    bool save_xmls = false;
    bool make_executable = true;

#ifndef CSCC_PORTABLE_MODE
    std::string include_path_gen() const {
        return cscc_include_path_gen();
    }

    std::string include_path_bin() const {
        return cscc_include_path_bin();
    }

    std::string include_path_src() const {
        return cscc_include_path_src();
    }

    std::filesystem::path sycl_runtime_library() const {
        return cscc_sycl_runtime_library();
    }
#endif

    std::string cc;
    std::string cxx;

    mutable std::string k_cc;
    [[nodiscard]] boost::leaf::result<std::string const&> cc_for_kernel() const;

    mutable std::string clang_format_;
    [[nodiscard]] boost::leaf::result<std::string const&> clang_format() const;

    /* compiler options */
    std::string fsanitize;
    bool debug;
    bool device_debug;
    char opt_level;
    std::vector<std::string> remainder;
    detail::str_map<std::string> defines;
    std::vector<std::string> include_dirs;
    std::vector<std::string> libraries;
    std::vector<std::string> library_dirs;
    bool host_openmp = false;

    /* preprocessor options */
    bool md, mmd, mm, m;
    std::string mf, mt;

    std::string mutable nvcc_;
    std::string cuda_library_path;
    std::string cuda_arch;

    [[nodiscard]] boost::leaf::result<std::string const&> nvcc() const;

    std::string mutable hipcc_;

    [[nodiscard]] boost::leaf::result<std::string const&> hipcc() const;

    [[nodiscard]] boost::leaf::result<void> validate();

    void begin_task(std::string_view) const;

    void task_msg(std::string_view) const;

    void end_task() const;

    unsigned task_id() const;

    [[nodiscard]] boost::leaf::result<void> ensure_executable(
        std::string& path, std::string_view extra_path = {}) const;

private:
    struct impl;
    std::unique_ptr<impl> pimpl_;

    void set_default_targets();
};

std::string find_command(std::string_view cmd, std::string_view path = {});
