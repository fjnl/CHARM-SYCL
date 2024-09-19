#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utils/errors.hpp>
#include <utils/io.hpp>

namespace errs = utils::errors;
namespace leaf = boost::leaf;
using boost::leaf::result;
using leaf::e_api_function;
using leaf::e_errno;
using leaf::e_file_name;

namespace {

[[nodiscard]] result<void> fix_io(int src_fd, int dst_fd) {
    auto const f = [&](int fd) -> result<void> {
        if (fd != dst_fd) {
            if (dup2(fd, dst_fd) == -1) {
                return errs::make_system_error("dup2");
            }
        }
        return {};
    };

    if (src_fd == -1) {
        auto fd = BOOST_LEAF_CHECK(utils::io::file_desc::create("/dev/null"));
        return f(fd.get());
    }
    return f(src_fd);
}

}  // namespace

namespace utils::io {

result<bool> fork_and_execv(std::string const& path, std::vector<std::string> const& args) {
    return fork_and_execv(path, args, exec_opts());
}

result<bool> fork_and_execv(std::string const& path, std::vector<std::string> const& args,
                            exec_opts const& opts) {
    auto loader = leaf::on_error(e_file_name{path});
    std::vector<char const*> argp;

    argp.reserve(args.size());
    for (auto const& arg : args) {
        argp.push_back(arg.c_str());
    }
    argp.push_back(nullptr);

    auto const pid = ::fork();
    if (pid == -1) {
        return errs::make_system_error("fork");
    }

    if (pid == 0) {
        file::disable_removing_tempfiles();

        errs::try_handle_some(
            argp[0],
            [&]() -> result<void> {
                if (opts.umask != static_cast<mode_t>(-1)) {
                    umask(opts.umask);
                }

                BOOST_LEAF_CHECK(fix_io(opts.stdin, STDIN_FILENO));
                BOOST_LEAF_CHECK(fix_io(opts.stdout, STDOUT_FILENO));
                BOOST_LEAF_CHECK(fix_io(opts.stderr, STDERR_FILENO));

                if (::execv(path.c_str(), const_cast<char* const*>(argp.data())) == -1) {
                    return errs::make_system_error("exec");
                }

                return {};
            },
            [&](boost::leaf::verbose_diagnostic_info const& diag) -> void {
                errs::report_and_exit(argp[0], diag);
            });
        std::abort();
    }

    int status;
    if (::waitpid(pid, &status, 0) == -1) {
        return errs::make_system_error("waitpid");
    }

    if (opts.check) {
        if (WIFSIGNALED(status)) {
            return make_io_error(subprocess_error::make_signaled(WTERMSIG(status)));
        } else if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                return make_io_error(subprocess_error::make_exited(WEXITSTATUS(status)));
            }
            return true;
        } else {
            std::abort();
        }
    } else {
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return false;
        }
        return true;
    }
}

}  // namespace utils::io
