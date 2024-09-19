#include "config.hpp"
#include "task.hpp"

boost::leaf::result<bool> run_self(config const& cfg, std::vector<std::string> const& args,
                                   utils::io::exec_opts const& opts) {
    if (cfg.verbose) {
        std::string msg = "$ ";
        for (auto const& arg : args) {
            msg += arg;
            msg += ' ';
        }
        cfg.task_msg(msg);
    }

    return utils::io::fork_and_execv(BOOST_LEAF_CHECK(utils::io::read_proc_self_exe()), args,
                                     opts);
}

boost::leaf::result<bool> run_command(config const& cfg, std::vector<std::string> const& args,
                                      utils::io::exec_opts const& opts) {
    if (cfg.verbose) {
        std::string msg = "$ ";
        for (auto const& arg : args) {
            msg += arg;
            msg += ' ';
        }
        cfg.task_msg(msg);
    }

    return utils::io::fork_and_execv(args.at(0), args, opts);
}
