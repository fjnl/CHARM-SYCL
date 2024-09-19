#pragma once

#include <iostream>
#include <charm/sycl/config.hpp>

#ifdef CSCC_USE_FMT_FORMAT
#    include <fmt/format.h>
#    include <fmt/ostream.h>
#else
#    include <format>
#    include <iterator>
#endif

CHARM_SYCL_BEGIN_NAMESPACE
namespace fmt {

#ifdef CSCC_USE_FMT_FORMAT
using ::fmt::format;
using ::fmt::format_string;
using ::fmt::print;
using ::fmt::ptr;
#else
using std::format;
using std::format_string;

template <class T>
void const* ptr(T* ptr) {
    return reinterpret_cast<void const*>(ptr);
}

template <class T>
void const* ptr(T const* ptr) {
    return reinterpret_cast<void const*>(ptr);
}

template <class... Args>
void print(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
    std::ostreambuf_iterator it(os);
    std::format_to(it, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
    print(std::cout, fmt, std::forward<Args>(args)...);
}
#endif

}  // namespace fmt
CHARM_SYCL_END_NAMESPACE

namespace format = CHARM_SYCL_NS::fmt;
