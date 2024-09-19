#include <fcntl.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <unistd.h>
#include <utils/errors.hpp>
#include <utils/io.hpp>

namespace errs = utils::errors;
namespace leaf = boost::leaf;
using boost::leaf::result;

namespace utils::io {

result<int> open(char const* filename, int flags, mode_t mode) {
    auto const fd = ::open(filename, flags, mode);
    if (fd < 0) {
        return make_io_error("open", leaf::e_file_name{filename});
    }
    return fd;
}

result<int> open(std::string const& filename, int flags, mode_t mode) {
    return open(filename.c_str(), flags, mode);
}

[[nodiscard]] boost::leaf::result<size_t> pread(int fd, void* buf, size_t len, off_t off) {
    auto const n = ::pread(fd, buf, len, off);
    if (n == -1) {
        return make_io_error("pread");
    }
    return static_cast<size_t>(n);
}

[[nodiscard]] boost::leaf::result<size_t> pwrite(int fd, void const* buf, size_t len,
                                                 off_t off) {
    auto const n = ::pwrite(fd, buf, len, off);
    if (n == -1) {
        return make_io_error("pwrite");
    }
    return static_cast<size_t>(n);
}

void report_and_exit(char const* argv0, io_error const&, boost::leaf::e_api_function const* fn,
                     boost::leaf::e_file_name const* file,
                     utils::errors::e_backtrace const* bt) {
    if (fn && file) {
        fmt::print(stderr, fg(fmt::color::red), "{}: IO Error: {}: {}\n", argv0, fn->value,
                   file->value);
    } else if (fn) {
        fmt::print(stderr, fg(fmt::color::red), "{}: IO Error: {}\n", argv0, fn->value);
    } else if (file) {
        fmt::print(stderr, fg(fmt::color::red), "{}: IO Error: {}\n", argv0, file->value);
    } else {
        fmt::print(stderr, fg(fmt::color::red), "{}: IO Error\n", argv0);
    }
    utils::errors::report_and_exit(bt);
}

void report_and_exit(char const* argv0, partial_write const&,
                     boost::leaf::e_api_function const* fn,
                     boost::leaf::e_file_name const* file,
                     utils::errors::e_backtrace const* bt) {
    if (fn && file) {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: Partial Write: {}: {}\n", argv0,
                   fn->value, file->value);
    } else if (fn) {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: Partial Write: {}\n", argv0,
                   fn->value);
    } else if (file) {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: Partial Write: {}\n", argv0,
                   file->value);
    } else {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: Partial Write\n", argv0);
    }
    utils::errors::report_and_exit(bt);
}

void report_and_exit(char const* argv0, unexpected_eof const&,
                     boost::leaf::e_api_function const* fn,
                     boost::leaf::e_file_name const* file,
                     utils::errors::e_backtrace const* bt) {
    if (fn && file) {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: Unexpected EOF: {}: {}\n", argv0,
                   fn->value, file->value);
    } else if (fn) {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: Unexpected EOF: {}\n", argv0,
                   fn->value);
    } else if (file) {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: Unexpected EOF: {}\n", argv0,
                   file->value);
    } else {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: Unexpected EOF\n", argv0);
    }
    utils::errors::report_and_exit(bt);
}

void report_and_exit(char const* argv0, subprocess_error const& err,
                     boost::leaf::e_api_function const* fn,
                     boost::leaf::e_file_name const* file,
                     utils::errors::e_backtrace const* bt) {
    fmt::print(stderr, fg(fmt::color::red), "{}: Subcommand Failed:", argv0);

    if (fn) {
        fmt::print(stderr, fg(fmt::color::red), " {}:", fn->value);
    }

    if (file) {
        fmt::print(stderr, fg(fmt::color::red), " {}:", file->value);
    }

    if (err.signaled) {
        fmt::print(stderr, fg(fmt::color::red), " Signal {}", err.signal);
    } else {
        fmt::print(stderr, fg(fmt::color::red), " Exited {}", err.exitstatus);
    }

    fmt::print(stderr, "\n");

    utils::errors::report_and_exit(bt);
}
}  // namespace utils::io
