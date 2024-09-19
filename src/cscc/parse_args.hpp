#include <iostream>
#include <boost/leaf.hpp>
#include <fmt/format.h>

struct config;

struct option_error {
    template <class... T>
    explicit option_error(fmt::format_string<T...> fmt, T&&... args)
        : msg(fmt::format(fmt, std::forward<T>(args)...)) {}

    std::string msg;

    friend std::ostream& operator<<(std::ostream& os, option_error const& err) {
        return os << "Option Error: " << err.msg;
    }
};

[[nodiscard]] boost::leaf::result<void> parse_args(int argc, char** argv, config& cfg);
