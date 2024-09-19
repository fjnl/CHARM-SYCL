#include <utils/io.hpp>
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

static char const* ext_for(cudafmt fmt) {
    switch (fmt) {
        case cudafmt::OBJECT:
            return ".o";
        case cudafmt::FATBIN:
            return ".fatbin";
        case cudafmt::PTX:
            return ".ptx";
        case cudafmt::CUBIN:
            return ".cubin";
    }
    std::terminate();
}

result<io::file> compile_kernel_cuda(config const& cfg, io::file const& input, cudafmt fmt) {
    cfg.begin_task("Compiling CUDA Kernels");

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(ext_for(fmt)));

    std::vector<std::string> cmd(
        {BOOST_LEAF_CHECK(cfg.nvcc()), "-c", "-o", out.filename(), input.filename()});

    cmd.push_back("-std=c++11");

    // FIXME:
    // Disable warning #550: set but unused veriable
    cmd.push_back("-diag-suppress");
    cmd.push_back("550");

    // FIXME:
    // Disable warning #177: declared but unused variable
    cmd.push_back("-diag-suppress");
    cmd.push_back("177");

    switch (fmt) {
        case cudafmt::FATBIN:
            cmd.push_back("--fatbin");
            break;
        case cudafmt::PTX:
            cmd.push_back("--ptx");
            break;
        case cudafmt::CUBIN:
            cmd.push_back("--cubin");
            break;
        default:
            break;
    }

    if (!cfg.cuda_arch.empty()) {
        cmd.push_back("-arch=" + cfg.cuda_arch);
    } else {
        cmd.push_back("-arch=sm_60");
    }

    cmd.push_back(std::string("-O") + cfg.opt_level);
    if (cfg.device_debug) {
        cmd.push_back("-g");
        cmd.push_back("-G");
    }

    BOOST_LEAF_CHECK(run_command(cfg, cmd));
    BOOST_LEAF_CHECK(save_temps(cfg, out, filetype::other));

    cfg.end_task();

    return out;
}
