#pragma once

#include <string>
#include <boost/leaf.hpp>

namespace utils::errors {

struct e_backtrace {
    std::string msg;
};

void load_backtrace(e_backtrace& e);

[[noreturn]] void report_and_exit(char const* argv0, boost::leaf::e_errno const& e,
                                  boost::leaf::e_api_function const& fn,
                                  boost::leaf::e_file_name const* file, e_backtrace const* bt);

[[noreturn]] void report_and_exit(char const* argv0,
                                  boost::leaf::verbose_diagnostic_info const& diag);

[[noreturn]] void report_and_exit(e_backtrace const* bt);

template <class F, class... Items>
auto try_handle_some(char const* argv0, F&& f, Items&&... items) {
    using result_type = std::decay_t<decltype(f().value())>;

    return boost::leaf::try_handle_some(
        std::forward<F>(f),
        [&](boost::leaf::e_errno const& e, boost::leaf::e_api_function const& fn,
            boost::leaf::e_file_name const* file, e_backtrace const* bt) -> result_type {
            report_and_exit(argv0, e, fn, file, bt);
        },
        std::forward<Items>(items)...);
}

template <class... Items>
auto make_system_error(char const* fn, Items&&... items) {
    return boost::leaf::new_error(
        boost::leaf::e_errno{}, boost::leaf::e_api_function{fn},
        [](utils::errors::e_backtrace& e) {
            utils::errors::load_backtrace(e);
        },
        std::forward<Items>(items)...);
}

}  // namespace utils::errors
