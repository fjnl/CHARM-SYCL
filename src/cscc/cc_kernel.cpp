#include "cc_kernel.hpp"
#include <utils/io.hpp>
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

result<cc_vendor> check_cc_vendor(config const& cfg) {
    auto cc = BOOST_LEAF_CHECK(cfg.cc_for_kernel());

    if (cc == "__clang__") {
        return cc_vendor::internal_clang;
    }

    auto version = BOOST_LEAF_CHECK(io::file::mktemp(".txt"));
    io::exec_opts opts;
    opts.stdout = version.fd();

    BOOST_LEAF_CHECK(run_command(cfg, {cc, "--version"}, opts));

    auto buffer = BOOST_LEAF_CHECK(version.readsome(0, 100));
    std::string_view ver(buffer.data(), buffer.size());

    if (ver.starts_with("gcc (GCC)")) {
        return cc_vendor::gcc;
    }
    if (ver.find("clang version") != std::string_view::npos) {
        return cc_vendor::clang;
    }
    return cc_vendor::gcc;
}

result<io::file> compile_kernel_cc(config const& cfg, io::file const& input,
                                   utils::target target) {
    auto out = BOOST_LEAF_CHECK(io::file::mktemp(".o"));

    cfg.begin_task("Compiling C Kernels");

    auto const cc_vendor = BOOST_LEAF_CHECK(check_cc_vendor(cfg));

    std::vector<std::string> cmd(
        {BOOST_LEAF_CHECK(cfg.cc_for_kernel()), "-c", "-o", out.filename(), input.filename()});

    if (is_openmp(target)) {
        switch (cc_vendor) {
            case cc_vendor::gcc:
                cmd.push_back("-fopenmp");
                break;

            case cc_vendor::clang:
            case cc_vendor::internal_clang:
                cmd.push_back("-fopenmp=libgomp");
                break;
        }
    }

    cmd.push_back(std::string("-O") + cfg.opt_level);
    if (cfg.device_debug) {
        cmd.push_back("-g");
    }
    if (!cfg.fsanitize.empty()) {
        cmd.push_back("-fsanitize=" + cfg.fsanitize);
    }

    if (cmd.front() == "__clang__") {
        cmd.push_back("--driver-mode=gcc");
#ifdef CSCC_PORTABLE_MODE
        cmd.push_back("-isystem/__clang__");
#endif
        BOOST_LEAF_CHECK(run_self(cfg, cmd));
    } else {
        BOOST_LEAF_CHECK(run_command(cfg, cmd));
    }
    BOOST_LEAF_CHECK(out.reopen());

    cfg.end_task();

    return out;
}
