#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

template <class Selector>
inline device select_device(Selector const&);

template <class E>
bool test_enum(E x, E y) {
    using T = std::underlying_type_t<E>;
    return static_cast<T>(x) & static_cast<T>(y);
}

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
