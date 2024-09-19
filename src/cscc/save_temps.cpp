#include <fmt/format.h>
#include <utils/io.hpp>
#include <utils/target.hpp>
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

result<void> save_temps_impl(config const& cfg, io::file& f, std::string const& outfile,
                             filetype type) {
    auto const do_save = cfg.save_temps || (cfg.save_kernels && type == filetype::kernel) ||
                         (cfg.save_xmls && type == filetype::xml);

    if (!do_save) {
        return {};
    }

    cfg.task_msg(fmt::format("save_temps: copy from {} to {}", f.filename(), outfile));

    auto out = BOOST_LEAF_CHECK(io::file::create(outfile));
    BOOST_LEAF_CHECK(out.copy_from(f));

    f = std::move(out);

    return {};
}

result<void> save_temps(config const& cfg, io::file& f, filetype type) {
    return save_temps(cfg, f, f.ext(), type);
}

result<void> save_temps(config const& cfg, io::file& f, std::string_view ext, filetype type) {
    return save_temps_impl(
        cfg, f, fmt::format("{}.{}{}", trim_ext(cfg.output), cfg.task_id(), ext), type);
}

result<void> save_temps(config const& cfg, io::file& f, utils::target t, filetype type) {
    return save_temps(cfg, f, f.ext(), t, type);
}

result<void> save_temps(config const& cfg, io::file& f, std::string_view ext, utils::target t,
                        filetype type) {
    return save_temps_impl(
        cfg, f,
        fmt::format("{}.{}.{}{}", trim_ext(cfg.output), cfg.task_id(), utils::show(t), ext),
        type);
}
