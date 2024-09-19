#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <boost/leaf/result.hpp>
#include <utils/io.hpp>
#include <utils/target.hpp>

struct config;

inline std::string_view trim_ext(std::string_view filename) {
    if (auto const pos = filename.rfind('.'); pos != std::string_view::npos) {
        return filename.substr(0, pos);
    }
    return filename;
}

template <class Timepoint>
double delta_sec(Timepoint const& begin, Timepoint const& end) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1.0e9;
}

[[nodiscard]] boost::leaf::result<bool> run_self(config const& cfg,
                                                 std::vector<std::string> const& args,
                                                 utils::io::exec_opts const& opts = {});

[[nodiscard]] boost::leaf::result<bool> run_command(config const& cfg,
                                                    std::vector<std::string> const& args,
                                                    utils::io::exec_opts const& opts = {});

enum class filetype { kernel, xml, other };

[[nodiscard]] boost::leaf::result<void> save_temps(config const& cfg, utils::io::file& f,
                                                   filetype type);

[[nodiscard]] boost::leaf::result<void> save_temps(config const& cfg, utils::io::file& f,
                                                   std::string_view ext, filetype type);

[[nodiscard]] boost::leaf::result<void> save_temps(config const& cfg, utils::io::file& f,
                                                   utils::target t, filetype type);

[[nodiscard]] boost::leaf::result<void> save_temps(config const& cfg, utils::io::file& f,
                                                   std::string_view ext, utils::target t,
                                                   filetype type);

[[nodiscard]] boost::leaf::result<bool> is_marked_object(utils::io::file const& file);

[[nodiscard]] boost::leaf::result<std::vector<utils::target>> targets_from_object(
    utils::io::file const& input_file);

[[nodiscard]] boost::leaf::result<utils::io::file> make_marked_object(
    config const& cfg, utils::io::file const& input_file, std::vector<utils::target> targets);

[[nodiscard]] boost::leaf::result<utils::io::file> extract_from_marked_object(
    config const& cfg, utils::io::file const& input);

[[nodiscard]] boost::leaf::result<utils::io::file> make_binary_loader_source(
    config const& cfg, utils::io::file const& file, std::string_view prefix,
    std::string_view kind);

[[nodiscard]] boost::leaf::result<utils::io::file> bin2asm(config const& cfg,
                                                           utils::io::file const& input,
                                                           std::string& prefix);

[[nodiscard]] boost::leaf::result<utils::io::file> run_cpp(config const& cfg,
                                                           utils::io::file const& input_file,
                                                           utils::io::file const* desc_file,
                                                           bool for_host);

[[nodiscard]] boost::leaf::result<utils::io::file> compile_host(config const& cfg,
                                                                utils::io::file const& input,
                                                                bool cc_mode, bool pic);

[[nodiscard]] boost::leaf::result<utils::io::file> compile_kernel_cc(
    config const& cfg, utils::io::file const& input, utils::target target);

[[nodiscard]] boost::leaf::result<utils::io::file> compile_kernel_hipcc(
    config const& cfg, utils::io::file const& input);

enum class cudafmt { OBJECT, FATBIN, PTX, CUBIN };

[[nodiscard]] boost::leaf::result<utils::io::file> compile_kernel_cuda(
    config const& cfg, utils::io::file const& input, cudafmt fmt);

[[nodiscard]] boost::leaf::result<void> link_exe(config const& cfg,
                                                 std::vector<utils::io::file>& inputs);

[[nodiscard]] boost::leaf::result<utils::io::file> link_so(
    config const& cfg, std::vector<utils::io::file> const& inputs, bool requires_openmp);

[[nodiscard]] boost::leaf::result<void> link_packed_file(
    config const& cfg, std::vector<utils::io::file> const& inputs);

[[nodiscard]] boost::leaf::result<std::vector<utils::io::file>> extract_packed_file(
    config const& cfg, utils::io::file const& input);

[[nodiscard]] boost::leaf::result<bool> is_packed_file(utils::io::file const& file);

[[nodiscard]] boost::leaf::result<utils::io::file> run_clang_format(
    utils::io::file const& input);

using file_map = std::unordered_multimap<utils::target, utils::io::file>;

[[nodiscard]] boost::leaf::result<std::pair<file_map, utils::io::file>> run_kext(
    config const& cfg, utils::io::file const& input_file, std::vector<std::string>& symbols);

[[nodiscard]] boost::leaf::result<file_map> run_lower(config const& cfg,
                                                      file_map const& input_files);

[[nodiscard]] boost::leaf::result<file_map> run_cback(config const& cfg,
                                                      file_map const& input_files);
