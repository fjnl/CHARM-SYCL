
#pragma once
#include <charm/sycl.hpp>
CHARM_SYCL_BEGIN_NAMESPACE
range<2>::range(size_t dim0, size_t dim1) {
    (*this)[0] = dim0;
    (*this)[1] = dim1;
}
size_t range<2>::size() const {
    return (*this)[0] * (*this)[1];
}
size_t range<2>::get(size_t dimension) const {
    return (*this)[dimension];
}
size_t& range<2>::operator[](size_t dimension) {
    return range_[dimension];
}
size_t const& range<2>::operator[](size_t dimension) const {
    return range_[dimension];
}
inline CHARM_SYCL_INLINE bool operator==(range<2> const& lhs, range<2> const& rhs) {
    return lhs[0] == rhs[0] && lhs[1] == rhs[1];
}
inline CHARM_SYCL_INLINE bool operator!=(range<2> const& lhs, range<2> const& rhs) {
    return lhs[0] != rhs[0] && lhs[1] != rhs[1];
}
inline CHARM_SYCL_INLINE range<2> operator+(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] + rhs[0], lhs[1] + rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator+(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs + rhs[0], lhs + rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator+(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] + rhs, lhs[1] + rhs);
}
inline CHARM_SYCL_INLINE range<2> operator-(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] - rhs[0], lhs[1] - rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator-(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs - rhs[0], lhs - rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator-(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] - rhs, lhs[1] - rhs);
}
inline CHARM_SYCL_INLINE range<2> operator*(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] * rhs[0], lhs[1] * rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator*(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs * rhs[0], lhs * rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator*(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] * rhs, lhs[1] * rhs);
}
inline CHARM_SYCL_INLINE range<2> operator/(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] / rhs[0], lhs[1] / rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator/(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs / rhs[0], lhs / rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator/(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] / rhs, lhs[1] / rhs);
}
inline CHARM_SYCL_INLINE range<2> operator%(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] % rhs[0], lhs[1] % rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator%(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs % rhs[0], lhs % rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator%(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] % rhs, lhs[1] % rhs);
}
inline CHARM_SYCL_INLINE range<2> operator<<(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] << rhs[0], lhs[1] << rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator<<(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs << rhs[0], lhs << rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator<<(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] << rhs, lhs[1] << rhs);
}
inline CHARM_SYCL_INLINE range<2> operator>>(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] >> rhs[0], lhs[1] >> rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator>>(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs >> rhs[0], lhs >> rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator>>(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] >> rhs, lhs[1] >> rhs);
}
inline CHARM_SYCL_INLINE range<2> operator&(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] & rhs[0], lhs[1] & rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator&(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs & rhs[0], lhs & rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator&(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] & rhs, lhs[1] & rhs);
}
inline CHARM_SYCL_INLINE range<2> operator|(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] | rhs[0], lhs[1] | rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator|(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs | rhs[0], lhs | rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator|(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] | rhs, lhs[1] | rhs);
}
inline CHARM_SYCL_INLINE range<2> operator^(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] ^ rhs[0], lhs[1] ^ rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator^(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs ^ rhs[0], lhs ^ rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator^(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] ^ rhs, lhs[1] ^ rhs);
}
inline CHARM_SYCL_INLINE range<2> operator&&(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] && rhs[0], lhs[1] && rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator&&(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs && rhs[0], lhs && rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator&&(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] && rhs, lhs[1] && rhs);
}
inline CHARM_SYCL_INLINE range<2> operator||(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] || rhs[0], lhs[1] || rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator||(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs || rhs[0], lhs || rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator||(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] || rhs, lhs[1] || rhs);
}
inline CHARM_SYCL_INLINE range<2> operator<(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] < rhs[0], lhs[1] < rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator<(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs < rhs[0], lhs < rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator<(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] < rhs, lhs[1] < rhs);
}
inline CHARM_SYCL_INLINE range<2> operator>(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] > rhs[0], lhs[1] > rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator>(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs > rhs[0], lhs > rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator>(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] > rhs, lhs[1] > rhs);
}
inline CHARM_SYCL_INLINE range<2> operator<=(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] <= rhs[0], lhs[1] <= rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator<=(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs <= rhs[0], lhs <= rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator<=(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] <= rhs, lhs[1] <= rhs);
}
inline CHARM_SYCL_INLINE range<2> operator>=(range<2> const& lhs, range<2> const& rhs) {
    return range<2>(lhs[0] >= rhs[0], lhs[1] >= rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator>=(size_t lhs, range<2> const& rhs) {
    return range<2>(lhs >= rhs[0], lhs >= rhs[1]);
}
inline CHARM_SYCL_INLINE range<2> operator>=(range<2> const& lhs, size_t rhs) {
    return range<2>(lhs[0] >= rhs, lhs[1] >= rhs);
}
inline CHARM_SYCL_INLINE range<2>& operator+=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] += rhs[0];
    lhs[1] += rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator+=(range<2>& lhs, size_t rhs) {
    lhs[0] += rhs;
    lhs[1] += rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator-=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] -= rhs[0];
    lhs[1] -= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator-=(range<2>& lhs, size_t rhs) {
    lhs[0] -= rhs;
    lhs[1] -= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator*=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] *= rhs[0];
    lhs[1] *= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator*=(range<2>& lhs, size_t rhs) {
    lhs[0] *= rhs;
    lhs[1] *= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator/=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] /= rhs[0];
    lhs[1] /= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator/=(range<2>& lhs, size_t rhs) {
    lhs[0] /= rhs;
    lhs[1] /= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator%=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] %= rhs[0];
    lhs[1] %= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator%=(range<2>& lhs, size_t rhs) {
    lhs[0] %= rhs;
    lhs[1] %= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator<<=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] <<= rhs[0];
    lhs[1] <<= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator<<=(range<2>& lhs, size_t rhs) {
    lhs[0] <<= rhs;
    lhs[1] <<= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator>>=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] >>= rhs[0];
    lhs[1] >>= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator>>=(range<2>& lhs, size_t rhs) {
    lhs[0] >>= rhs;
    lhs[1] >>= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator&=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] &= rhs[0];
    lhs[1] &= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator&=(range<2>& lhs, size_t rhs) {
    lhs[0] &= rhs;
    lhs[1] &= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator|=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] |= rhs[0];
    lhs[1] |= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator|=(range<2>& lhs, size_t rhs) {
    lhs[0] |= rhs;
    lhs[1] |= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator^=(range<2>& lhs, range<2> const& rhs) {
    lhs[0] ^= rhs[0];
    lhs[1] ^= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE range<2>& operator^=(range<2>& lhs, size_t rhs) {
    lhs[0] ^= rhs;
    lhs[1] ^= rhs;
    return lhs;
}
CHARM_SYCL_END_NAMESPACE
