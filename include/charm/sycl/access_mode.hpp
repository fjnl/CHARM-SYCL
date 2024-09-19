#pragma once
#include <charm/sycl/config.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

enum class access_mode {
    // clang-format off
    read               = 0x4 |       0x1,
    write              = 0x4 | 0x2,
    read_write         = 0x4 | 0x2 | 0x1,
    discard_write      =       0x2,
    discard_read_write =       0x2 | 0x1,
    // clang-format on
    atomic /* not implemented */ = 0x8,
};

namespace detail {

inline constexpr bool is_readable(access_mode mode) {
    return mode == access_mode::read || mode == access_mode::read_write ||
           mode == access_mode::discard_read_write;
}

inline constexpr bool is_writable(access_mode mode) {
    return mode == access_mode::write || mode == access_mode::read_write ||
           mode == access_mode::discard_write || mode == access_mode::discard_read_write;
}

}  // namespace detail

enum class target {
    device,
    local,
    host_buffer,
    global_buffer = device,
    constant_buffer /* not implemented */,
};

namespace access {

using mode = sycl::access_mode;
using target = sycl::target;

enum class fence_space : char {
    local_space,
    global_space,
    global_and_local,
};

}  // namespace access

namespace detail {

struct read_only_t {};

struct read_write_t {};

struct write_only_t {};

// clang-format off
template <access_mode AccessMode>
using to_tag_t =
    std::conditional_t<AccessMode == access_mode::read, read_only_t,
    std::conditional_t<AccessMode == access_mode::read_write, read_write_t,
    std::conditional_t<AccessMode == access_mode::write, write_only_t,
    void>>>;
// clang-format on

template <class TagT>
struct to_mode;

template <>
struct to_mode<read_only_t> {
    static constexpr auto value = access_mode::read;
};

template <>
struct to_mode<write_only_t> {
    static constexpr auto value = access_mode::discard_write;
};

template <>
struct to_mode<read_write_t> {
    static constexpr auto value = access_mode::read_write;
};

template <class TagT>
auto constexpr to_mode_v = to_mode<TagT>::value;

template <class TagT>
auto constexpr is_tag_v =
    std::is_same_v<TagT, read_only_t> || std::is_same_v<TagT, write_only_t> ||
    std::is_same_v<TagT, read_write_t>;

}  // namespace detail

inline constexpr detail::read_only_t read_only;

inline constexpr detail::read_write_t read_write;

inline constexpr detail::write_only_t write_only;

CHARM_SYCL_END_NAMESPACE
