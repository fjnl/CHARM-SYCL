#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

inline sycl::id<3> extend(sycl::id<1> const& r) {
    return {0, 0, r[0]};
}

inline sycl::id<3> extend(sycl::id<2> const& r) {
    return {0, r[0], r[1]};
}

inline sycl::id<3> extend(sycl::id<3> const& r) {
    return r;
}

inline sycl::range<3> extend(sycl::range<1> const& r) {
    return {1, 1, r[0]};
}

inline sycl::range<3> extend(sycl::range<2> const& r) {
    return {1, r[0], r[1]};
}

inline sycl::range<3> extend(sycl::range<3> const& r) {
    return r;
}

inline sycl::nd_range<3> extend(sycl::nd_range<1> const& ndr) {
    return sycl::nd_range<3>(extend(ndr.get_global_range()), extend(ndr.get_local_range()));
}

inline sycl::nd_range<3> extend(sycl::nd_range<2> const& ndr) {
    return sycl::nd_range<3>(extend(ndr.get_global_range()), extend(ndr.get_local_range()));
}

inline sycl::nd_range<3> extend(sycl::nd_range<3> const& ndr) {
    return ndr;
}

template <int _Dim>
inline sycl::range<_Dim> shrink(sycl::range<3> const& r) {
    if constexpr (_Dim == 1) {
        return sycl::range<_Dim>(r[2]);
    } else if constexpr (_Dim == 2) {
        return sycl::range<_Dim>(r[1], r[2]);
    } else {
        return r;
    }
}

template <int _Dim>
inline sycl::id<_Dim> shrink(sycl::id<3> const& r) {
    if constexpr (_Dim == 1) {
        return sycl::id<_Dim>(r[2]);
    } else if constexpr (_Dim == 2) {
        return sycl::id<_Dim>(r[1], r[2]);
    } else {
        return r;
    }
}

template <int Dimensions, class Range, class Id>
CHARM_SYCL_INLINE inline size_t linear_id(Range const& range, Id const& id) {
    if constexpr (Dimensions == 1) {
        return id[0];
    } else if constexpr (Dimensions == 2) {
        return id[1] + id[0] * range[1];
    } else if constexpr (Dimensions == 3) {
        return id[2] + id[1] * range[2] + id[0] * range[1] * range[2];
    } else {
        static_assert(Dimensions == 1 || Dimensions == 2 || Dimensions == 3,
                      "Dimesions must be 1, 2, or 3.");
    }
}

template <int Dimensions, class Range>
CHARM_SYCL_INLINE inline auto linear_range(Range const& range) {
    if constexpr (Dimensions == 1) {
        return range[0];
    } else if constexpr (Dimensions == 2) {
        return range[1] * range[0];
    } else if constexpr (Dimensions == 3) {
        return range[2] * range[1] * range[0];
    } else {
        static_assert(Dimensions == 1 || Dimensions == 2 || Dimensions == 3,
                      "Dimesions must be 1, 2, or 3.");
    }
}

}  // namespace detail

inline void throw_error(errc e, char const* what) {
    throw exception(make_error_code(e), what);
}

inline void unimplemented() {
    throw exception(make_error_code(errc::unimplemented), "not implemented");
}

namespace runtime {

#ifndef __SYCL_DEVICE_ONLY__
CHARM_SYCL_HOST_INLINE void __charm_sycl_assume(bool cond) {
    __builtin_assume(cond);
}
#endif

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
