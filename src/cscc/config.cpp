#include "config.h"
#include <chrono>
#include <filesystem>
#include <vector>
#include <boost/assert.hpp>
#include <fmt/format.h>
#include "config.hpp"
#include "task.hpp"

#ifndef CSCC_PORTABLE_MODE
std::filesystem::path cscc_include_path_gen() {
#    ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_SOURCE_DIR) / "generated" / "include";
#    else
    return "";
#    endif
}

std::filesystem::path cscc_include_path_bin() {
#    ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_BINARY_DIR) / "include";
#    else
    return std::filesystem::path(CMAKE_INSTALL_PREFIX) / "include";
#    endif
}

std::filesystem::path cscc_include_path_src() {
#    ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_SOURCE_DIR) / "include";
#    else
    return "";
#    endif
}

std::filesystem::path cscc_sycl_runtime_library() {
#    ifndef CMAKE_INSTALL_PREFIX
    return std::filesystem::path(PROJECT_BINARY_DIR) / RUNTIME_LIBRARY;
#    else
    auto const filename = std::filesystem::path(RUNTIME_LIBRARY).filename();
    return std::filesystem::path(CMAKE_INSTALL_PREFIX) / "lib" / filename;
#    endif
}
#endif

struct config::impl {
    struct state {
        unsigned id;
        std::chrono::high_resolution_clock::time_point start;
    };

    std::vector<state> tasks_;
    unsigned task_id_ = 0;
};

#ifdef CSCC_PORTABLE_MODE
#    define IF_PORTABLE(then_, else_) then_
#else
#    define IF_PORTABLE(then_, else_) else_
#endif

config::config() : pimpl_(std::make_unique<impl>()) {
    cc = IF_PORTABLE("__clang__", CMAKE_C_COMPILER);
    cxx = IF_PORTABLE("__clang__", CMAKE_CXX_COMPILER);
    clang_format_ = IF_PORTABLE("clang-format", CLANG_FORMAT_COMMAND);

    debug = false;
    device_debug = false;
    opt_level = '0';

    md = false;
    mmd = false;
    mm = false;
    m = false;

#ifdef CSCC_PORTABLE_MODE
    nvcc_ = "nvcc";
#elif defined(CMAKE_CUDA_COMPILER)
    nvcc_ = CMAKE_CUDA_COMPILER;
#endif

    hipcc_ = IF_PORTABLE("hipcc", HIP_HIPCC_EXECUTABLE);
}

config::~config() = default;

void config::begin_task(std::string_view msg) const {
    auto const tid = ++pimpl_->task_id_;
    auto const time = std::chrono::high_resolution_clock::now();

    pimpl_->tasks_.push_back({tid, time});

    if (this->verbose) {
        std::string spc;
        for (size_t i = 1; i < pimpl_->tasks_.size(); i++) {
            spc += "  ";
        }
        fmt::print(stderr, "{}{:4d}: Start {}\n", spc, tid, msg);
    }
}

void config::task_msg(std::string_view msg) const {
    if (this->verbose) {
        std::string spc;
        for (size_t i = 0; i < pimpl_->tasks_.size(); i++) {
            spc += "  ";
        }
        fmt::print(stderr, "{}{:4d}:   {}\n", spc, task_id(), msg);
    }
}

void config::end_task() const {
    BOOST_ASSERT(!pimpl_->tasks_.empty());

    auto const t_end = std::chrono::high_resolution_clock::now();
    auto const t_start = pimpl_->tasks_.back().start;

    if (this->verbose) {
        std::string spc;
        for (size_t i = 1; i < pimpl_->tasks_.size(); i++) {
            spc += "  ";
        }
        fmt::print(stderr, "{}{:4d}: Finished in {:.2f} sec\n", spc, task_id(),
                   delta_sec(t_start, t_end));
    }

    pimpl_->tasks_.pop_back();
}

unsigned config::task_id() const {
    if (pimpl_->tasks_.empty()) {
        return 0;
    }
    return pimpl_->tasks_.back().id;
}

boost::leaf::result<void> config::validate() {
    if (show_version) {
        return {};
    }

    if (inputs.empty()) {
        return BOOST_LEAF_NEW_ERROR(config_error("Error: No input file"));
    }

    if (make_executable) {
        if (output.empty()) {
            output = "a.out";
        }
    } else {
        if (inputs.size() > 1) {
            return BOOST_LEAF_NEW_ERROR(
                config_error("Error: too many inputs: {}", inputs.size()));
        }
        if (output.empty()) {
            auto i = std::filesystem::path(inputs.front());
            auto o = i.filename().stem();
            o += ".o";
            output = o.string();
        }
    }

    if (k_cc.empty()) {
        k_cc = cc;
    }

    if (targets.empty()) {
        set_default_targets();
    }

    return {};
}

boost::leaf::result<std::string const&> config::cc_for_kernel() const {
    if (k_cc.empty()) {
        k_cc = IF_PORTABLE("__clang__", CMAKE_C_COMPILER);
    }
    return k_cc;
    /*
        if (!k_cc.empty()) {
            BOOST_LEAF_CHECK(ensure_executable(k_cc));
            return k_cc;
        }

        if (auto env_cc = getenv("CC")) {
            std::string cmd(env_cc);
            if (ensure_executable(cmd)) {
                k_cc = cmd;
                return k_cc;
            }
            return BOOST_LEAF_NEW_ERROR(config_error("command not found: {}", env_cc));
        }

        std::string gcc("gcc");
        if (ensure_executable(gcc)) {
            k_cc = gcc;
            return k_cc;
        }

        std::string clang("clang");
        if (ensure_executable(clang)) {
            k_cc = clang;
            return k_cc;
        }

        std::string cc("cc");
        if (ensure_executable(cc)) {
            k_cc = cc;
            return k_cc;
        }

        return BOOST_LEAF_NEW_ERROR(config_error("command not found: gcc, clang, cc"));
        */
}

boost::leaf::result<std::string const&> config::nvcc() const {
    BOOST_LEAF_CHECK(ensure_executable(nvcc_, "/usr/local/cuda/bin"));
    return nvcc_;
}

boost::leaf::result<std::string const&> config::hipcc() const {
    BOOST_LEAF_CHECK(ensure_executable(hipcc_, "/opt/rocm/bin"));
    return hipcc_;
}

boost::leaf::result<std::string const&> config::clang_format() const {
    BOOST_LEAF_CHECK(ensure_executable(clang_format_));
    return clang_format_;
}

boost::leaf::result<void> config::ensure_executable(std::string& command,
                                                    std::string_view extra_path) const {
    auto resolved = find_command(command);

    if (resolved.empty() && !extra_path.empty()) {
        resolved = find_command(command, extra_path);
    }

    if (resolved.empty()) {
        return BOOST_LEAF_NEW_ERROR(config_error("command not found: {}", command));
    }

    if (command != resolved) {
        task_msg(fmt::format("{} found: {}", command, resolved));
    }

    command = std::move(resolved);

    return {};
}

void config::set_default_targets() {
    std::vector<utils::target> result;
    std::optional<bool> cuda_found, hip_found;

    for (auto const& t : utils::default_targets()) {
        if (is_cuda(t)) {
            if (!cuda_found.has_value()) {
                cuda_found = nvcc().has_value();
            }

            if (*cuda_found) {
                result.push_back(t);
            }
        } else if (is_hip(t)) {
            if (!hip_found.has_value()) {
                hip_found = hipcc().has_value();
            }
            if (*hip_found) {
                result.push_back(t);
            }
        } else {
            result.push_back(t);
        }
    }

    this->targets = result;
}

std::string find_command(std::string_view cmd, std::string_view path) {
    namespace fs = std::filesystem;
    using namespace std::literals;

    fs::path p(cmd);

    if (p.is_absolute()) {
        return p.string();
    }

    if (cmd.find('/') != std::string_view::npos) {
        p = fs::weakly_canonical(p);
        return p.string();
    }

    if (path.empty()) {
        auto const path_p = getenv("PATH");
        if (path_p) {
            path = path_p;
        }
    }

    if (!path.empty()) {
        size_t pos = 0;
        while (pos < path.size()) {
            auto colon = path.find(":", pos);

            if (colon == std::string_view::npos) {
                colon = path.size();
            }

            if (pos != colon) {
                auto dir_s = path.substr(pos, colon - pos);
                auto p = fs::weakly_canonical(fs::path(dir_s));
                p /= cmd;

                std::error_code ec;
                auto st = fs::status(p, ec);

                if (!ec && st.type() == fs::file_type::regular &&
                    (st.permissions() & (fs::perms::owner_exec | fs::perms::group_exec |
                                         fs ::perms::others_exec)) != fs::perms::none) {
                    return p.string();
                }
            }

            pos = colon;
            if (pos < path.size()) {
                pos++;
            }
        }
    }

    return {};
}
