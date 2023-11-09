#pragma once
#include <charm/sycl/config.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace detail {

/*
 * http://www.isthe.com/chongo/tech/comp/fnv/index.html#FNV-source
 * https://tools.ietf.org/html/draft-eastlake-fnv-17.html
 */
constexpr inline uint32_t fnv1a(char const* str) {
    uint32_t constexpr FNV_prime = UINT32_C(16777619);
    uint32_t constexpr FNV_bias = UINT32_C(2166136261);

    uint32_t h = FNV_bias;
    char c = 0;
    while ((c = *str++)) {
        h = h ^ c;
        h = h * FNV_prime;
    }

    return h;
}

constexpr inline uint32_t fnv1a(char const* str, size_t len) {
    uint32_t constexpr FNV_prime = UINT32_C(16777619);
    uint32_t constexpr FNV_bias = UINT32_C(2166136261);

    uint32_t h = FNV_bias;
    char c = 0;
    while (len > 0) {
        c = *str++;
        len--;
        h = h ^ c;
        h = h * FNV_prime;
    }

    return h;
}

/*
 * https://tools.ietf.org/html/draft-eastlake-fnv-17.html
 */
static_assert(fnv1a("") == UINT32_C(0x811c9dc5));
static_assert(fnv1a("a") == UINT32_C(0xe40c292c));
static_assert(fnv1a("foobar") == UINT32_C(0xbf9cf968));

static_assert(fnv1a("", 0) == UINT32_C(0x811c9dc5));
static_assert(fnv1a("a", 1) == UINT32_C(0xe40c292c));
static_assert(fnv1a("foobar", 6) == UINT32_C(0xbf9cf968));

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
