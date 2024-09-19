#include <utils/io.hpp>
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

result<io::file> compile_kernel_hipcc(config const& cfg, io::file const& input) {
    cfg.begin_task("Compiling HIP Kernels");

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(".hsaco"));

    std::vector<std::string> cmd(
        {BOOST_LEAF_CHECK(cfg.hipcc()), "--genco", "-o", out.filename(), input.filename()});

    cmd.push_back("-std=c++17");
    cmd.push_back(std::string("-O") + cfg.opt_level);
    if (cfg.device_debug) {
        cmd.push_back("-g");
    }

    BOOST_LEAF_CHECK(run_command(cfg, cmd));
    BOOST_LEAF_CHECK(save_temps(cfg, out, filetype::other));

    cfg.end_task();

    return out;
}
