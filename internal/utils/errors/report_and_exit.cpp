#include <iostream>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <utils/errors.hpp>

namespace utils::errors {

[[noreturn]] void report_and_exit(e_backtrace const* bt) {
    if (bt) {
        fmt::print(stderr, fg(fmt::color::yellow), "-------\n");
        fmt::print(stderr, "{}", bt->msg.c_str());
        fmt::print(stderr, fg(fmt::color::yellow), "-------\n");
    }
    std::exit(1);
}

void report_and_exit(char const* argv0, boost::leaf::e_errno const& e,
                     boost::leaf::e_api_function const& fn,
                     boost::leaf::e_file_name const* file, e_backtrace const* bt) {
    if (file) {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: {}: {}: {}\n", argv0, fn.value,
                   ::strerror(e.value), file->value.c_str());
    } else {
        fmt::print(stderr, fg(fmt::color::red), "{}: Error: {}: {}\n", argv0, fn.value,
                   ::strerror(e.value));
    }
    report_and_exit(bt);
}

void report_and_exit(char const* argv0, boost::leaf::verbose_diagnostic_info const& diag) {
    fmt::print(stderr, fg(fmt::color::red), "{}: Unknown Error:\n{}\n", argv0,
               fmt::streamed(diag));

    std::exit(1);
}

}  // namespace utils::errors
