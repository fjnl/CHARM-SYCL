
#pragma once
#include <charm/sycl.hpp>
CHARM_SYCL_BEGIN_NAMESPACE
range<1>::range(size_t dim0) {
    (*this)[0] = dim0;
}
size_t range<1>::size() const {
    return (*this)[0];
}
size_t range<1>::get(size_t dimension) const {
    return (*this)[dimension];
}
size_t& range<1>::operator[](size_t dimension) {
    return range_[dimension];
}
size_t const& range<1>::operator[](size_t dimension) const {
    return range_[dimension];
}
inline CHARM_SYCL_INLINE bool operator==(range<1> const& lhs, range<1> const& rhs) {
    return lhs[0] == rhs[0];
}
inline CHARM_SYCL_INLINE bool operator!=(range<1> const& lhs, range<1> const& rhs) {
    return lhs[0] != rhs[0];
}
inline CHARM_SYCL_INLINE range<1> operator+(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] + rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator+(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs + rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator+(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] + rhs);
}
inline CHARM_SYCL_INLINE range<1> operator-(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] - rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator-(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs - rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator-(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] - rhs);
}
inline CHARM_SYCL_INLINE range<1> operator*(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] * rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator*(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs * rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator*(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] * rhs);
}
inline CHARM_SYCL_INLINE range<1> operator/(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] / rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator/(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs / rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator/(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] / rhs);
}
inline CHARM_SYCL_INLINE range<1> operator%(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] % rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator%(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs % rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator%(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] % rhs);
}
inline CHARM_SYCL_INLINE range<1> operator<<(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] << rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator<<(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs << rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator<<(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] << rhs);
}
inline CHARM_SYCL_INLINE range<1> operator>>(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] >> rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator>>(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs >> rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator>>(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] >> rhs);
}
inline CHARM_SYCL_INLINE range<1> operator&(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] & rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator&(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs & rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator&(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] & rhs);
}
inline CHARM_SYCL_INLINE range<1> operator|(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] | rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator|(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs | rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator|(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] | rhs);
}
inline CHARM_SYCL_INLINE range<1> operator^(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] ^ rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator^(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs ^ rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator^(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] ^ rhs);
}
inline CHARM_SYCL_INLINE range<1> operator&&(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] && rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator&&(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs && rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator&&(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] && rhs);
}
inline CHARM_SYCL_INLINE range<1> operator||(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] || rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator||(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs || rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator||(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] || rhs);
}
inline CHARM_SYCL_INLINE range<1> operator<(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] < rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator<(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs < rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator<(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] < rhs);
}
inline CHARM_SYCL_INLINE range<1> operator>(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] > rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator>(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs > rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator>(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] > rhs);
}
inline CHARM_SYCL_INLINE range<1> operator<=(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] <= rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator<=(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs <= rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator<=(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] <= rhs);
}
inline CHARM_SYCL_INLINE range<1> operator>=(range<1> const& lhs, range<1> const& rhs) {
    return range<1>(lhs[0] >= rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator>=(size_t lhs, range<1> const& rhs) {
    return range<1>(lhs >= rhs[0]);
}
inline CHARM_SYCL_INLINE range<1> operator>=(range<1> const& lhs, size_t rhs) {
    return range<1>(lhs[0] >= rhs);
}
inline CHARM_SYCL_INLINE range<1>& operator+=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] += rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator+=(range<1>& lhs, size_t rhs) {
    lhs[0] += rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator-=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] -= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator-=(range<1>& lhs, size_t rhs) {
    lhs[0] -= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator*=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] *= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator*=(range<1>& lhs, size_t rhs) {
    lhs[0] *= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator/=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] /= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator/=(range<1>& lhs, size_t rhs) {
    lhs[0] /= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator%=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] %= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator%=(range<1>& lhs, size_t rhs) {
    lhs[0] %= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator<<=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] <<= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator<<=(range<1>& lhs, size_t rhs) {
    lhs[0] <<= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator>>=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] >>= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator>>=(range<1>& lhs, size_t rhs) {
    lhs[0] >>= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator&=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] &= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator&=(range<1>& lhs, size_t rhs) {
    lhs[0] &= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator|=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] |= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator|=(range<1>& lhs, size_t rhs) {
    lhs[0] |= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator^=(range<1>& lhs, range<1> const& rhs) {
    lhs[0] ^= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE range<1>& operator^=(range<1>& lhs, size_t rhs) {
    lhs[0] ^= rhs;
    return lhs;
}
CHARM_SYCL_END_NAMESPACE
