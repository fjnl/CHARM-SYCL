#include <vector>
#include <utils/io.hpp>
#include "cc_kernel.hpp"
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

result<utils::io::file> compile_host(config const& cfg, io::file const& input, bool cc_mode,
                                     bool pic) {
    cfg.begin_task("Compiling Host Code");

    auto out = BOOST_LEAF_CHECK(utils::io::file::mktemp(".o"));
    auto const is_asm = input.filename().ends_with(".s");

    std::vector<std::string> cmd(
        {cfg.cxx, "-o", out.filename().c_str(), "-c", input.filename()});

    if (cc_mode) {
        cmd.push_back("--driver-mode=gcc");
    } else {
        cmd.push_back("--driver-mode=g++");
        cmd.push_back("-Xclang");
        cmd.push_back("-fsycl-is-host");
        cmd.push_back("-std=c++20");
    }
    if (pic) {
        cmd.push_back("-fPIC");
    }
    if (cfg.host_openmp) {
        switch (BOOST_LEAF_CHECK(check_cc_vendor(cfg))) {
            case cc_vendor::gcc:
                cmd.push_back("-lgomp");
                break;

            case cc_vendor::clang:
            case cc_vendor::internal_clang:
                cmd.push_back("-fopenmp=libgomp");
                break;
        }
    }

#ifdef CSCC_USE_LIBCXX
    cmd.push_back(fmt::format("-stdlib=libc++"));
#endif
#ifdef CSCC_PORTABLE_MODE
    cmd.push_back("-I/__clang__");
#endif

    cmd.push_back(std::string("-O") + cfg.opt_level);
    if (cfg.debug) {
        cmd.push_back("-g");
    }
    if (!cfg.fsanitize.empty()) {
        cmd.push_back("-fsanitize=" + cfg.fsanitize);
    }
    for (auto const& dir : cfg.include_dirs) {
        cmd.push_back("-I" + dir);
    }
    if (!is_asm) {
        for (auto const& [k, v] : cfg.defines) {
            if (v.empty()) {
                cmd.push_back(std::string("-D") + k);
            } else {
                cmd.push_back(std::string("-D") + k + "=" + v);
            }
        }
    }

    if (cmd.front() == "__clang__") {
        BOOST_LEAF_CHECK(run_self(cfg, cmd));
    } else {
        BOOST_LEAF_CHECK(run_command(cfg, cmd));
    }
    BOOST_LEAF_CHECK(out.reopen());

    cfg.end_task();

    return out;
}

[[nodiscard]] result<io::file> run_cpp(config const& cfg, io::file const& input,
                                       io::file const* desc_file, bool for_host) {
    cfg.begin_task("Running Preprocessor");

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(input.ext()));

    std::vector<std::string> cmd({cfg.cxx, "-E", "-Xclang",
                                  for_host ? "-fsycl-is-host" : "-fsycl-is-device",
                                  "-std=c++20", "-w", "-o", out.filename(), input.filename()});

    if (desc_file) {
        cmd.push_back("-include");
        cmd.push_back(desc_file->filename());
    }

    cmd.push_back("--driver-mode=cpp");

#ifdef CSCC_USE_LIBCXX
    cmd.push_back(fmt::format("-stdlib=libc++"));
#endif

#ifdef CSCC_PORTABLE_MODE
    cmd.push_back("-I/__runtime__");
    cmd.push_back("-isystem/__clang__");
#else
    if (auto const inc = cfg.include_path_src(); !inc.empty()) {
        cmd.push_back("-I" + inc);
    }
    if (auto const inc = cfg.include_path_bin(); !inc.empty()) {
        cmd.push_back("-I" + inc);
    }
    if (auto const inc = cfg.include_path_gen(); !inc.empty()) {
        cmd.push_back("-I" + inc);
    }
#endif

    if (cfg.host_openmp) {
        cmd.push_back("-D_OPENMP");
    }

    for (auto const& dir : cfg.include_dirs) {
        cmd.push_back("-I" + dir);
    }
    for (auto const& [k, v] : cfg.defines) {
        if (v.empty()) {
            cmd.push_back(std::string("-D") + k);
        } else {
            cmd.push_back(std::string("-D") + k + "=" + v);
        }
    }
    if (for_host) {
        if (cfg.md) {
            cmd.push_back("-MD");
        }
        if (cfg.mmd) {
            cmd.push_back("-MMD");
        }
        if (cfg.mm) {
            cmd.push_back("-MM");
        }
        if (cfg.m) {
            cmd.push_back("-M");
        }
        if (!cfg.mf.empty()) {
            cmd.push_back("-MF");
            cmd.push_back(cfg.mf);
        }
        if (!cfg.mt.empty()) {
            cmd.push_back("-MT");
            cmd.push_back(cfg.mt);
        }
    }

    if (cmd.front() == "__clang__") {
        BOOST_LEAF_CHECK(run_self(cfg, cmd));
    } else {
        BOOST_LEAF_CHECK(run_command(cfg, cmd));
    }
    BOOST_LEAF_CHECK(out.reopen());
    BOOST_LEAF_CHECK(save_temps(cfg, out, filetype::other));

    cfg.end_task();

    return out;
}
