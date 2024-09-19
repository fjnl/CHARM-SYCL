
#pragma once
#include <charm/sycl.hpp>
CHARM_SYCL_BEGIN_NAMESPACE
range<3>::range(size_t dim0, size_t dim1, size_t dim2) {
    (*this)[0] = dim0;
    (*this)[1] = dim1;
    (*this)[2] = dim2;
}
size_t range<3>::size() const {
    return (*this)[0] * (*this)[1] * (*this)[2];
}
size_t range<3>::get(size_t dimension) const {
    return (*this)[dimension];
}
size_t& range<3>::operator[](size_t dimension) {
    return range_[dimension];
}
size_t const& range<3>::operator[](size_t dimension) const {
    return range_[dimension];
}
inline CHARM_SYCL_INLINE bool operator==(range<3> const& lhs, range<3> const& rhs) {
    return lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2];
}
inline CHARM_SYCL_INLINE bool operator!=(range<3> const& lhs, range<3> const& rhs) {
    return lhs[0] != rhs[0] && lhs[1] != rhs[1] && lhs[2] != rhs[2];
}
inline CHARM_SYCL_INLINE range<3> operator+(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator+(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs + rhs[0], lhs + rhs[1], lhs + rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator+(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] + rhs, lhs[1] + rhs, lhs[2] + rhs);
}
inline CHARM_SYCL_INLINE range<3> operator-(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator-(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs - rhs[0], lhs - rhs[1], lhs - rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator-(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] - rhs, lhs[1] - rhs, lhs[2] - rhs);
}
inline CHARM_SYCL_INLINE range<3> operator*(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator*(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs * rhs[0], lhs * rhs[1], lhs * rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator*(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] * rhs, lhs[1] * rhs, lhs[2] * rhs);
}
inline CHARM_SYCL_INLINE range<3> operator/(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] / rhs[0], lhs[1] / rhs[1], lhs[2] / rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator/(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs / rhs[0], lhs / rhs[1], lhs / rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator/(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] / rhs, lhs[1] / rhs, lhs[2] / rhs);
}
inline CHARM_SYCL_INLINE range<3> operator%(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] % rhs[0], lhs[1] % rhs[1], lhs[2] % rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator%(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs % rhs[0], lhs % rhs[1], lhs % rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator%(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] % rhs, lhs[1] % rhs, lhs[2] % rhs);
}
inline CHARM_SYCL_INLINE range<3> operator<<(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] << rhs[0], lhs[1] << rhs[1], lhs[2] << rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator<<(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs << rhs[0], lhs << rhs[1], lhs << rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator<<(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] << rhs, lhs[1] << rhs, lhs[2] << rhs);
}
inline CHARM_SYCL_INLINE range<3> operator>>(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] >> rhs[0], lhs[1] >> rhs[1], lhs[2] >> rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator>>(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs >> rhs[0], lhs >> rhs[1], lhs >> rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator>>(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] >> rhs, lhs[1] >> rhs, lhs[2] >> rhs);
}
inline CHARM_SYCL_INLINE range<3> operator&(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] & rhs[0], lhs[1] & rhs[1], lhs[2] & rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator&(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs & rhs[0], lhs & rhs[1], lhs & rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator&(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] & rhs, lhs[1] & rhs, lhs[2] & rhs);
}
inline CHARM_SYCL_INLINE range<3> operator|(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] | rhs[0], lhs[1] | rhs[1], lhs[2] | rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator|(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs | rhs[0], lhs | rhs[1], lhs | rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator|(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] | rhs, lhs[1] | rhs, lhs[2] | rhs);
}
inline CHARM_SYCL_INLINE range<3> operator^(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] ^ rhs[0], lhs[1] ^ rhs[1], lhs[2] ^ rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator^(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs ^ rhs[0], lhs ^ rhs[1], lhs ^ rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator^(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] ^ rhs, lhs[1] ^ rhs, lhs[2] ^ rhs);
}
inline CHARM_SYCL_INLINE range<3> operator&&(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] && rhs[0], lhs[1] && rhs[1], lhs[2] && rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator&&(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs && rhs[0], lhs && rhs[1], lhs && rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator&&(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] && rhs, lhs[1] && rhs, lhs[2] && rhs);
}
inline CHARM_SYCL_INLINE range<3> operator||(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] || rhs[0], lhs[1] || rhs[1], lhs[2] || rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator||(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs || rhs[0], lhs || rhs[1], lhs || rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator||(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] || rhs, lhs[1] || rhs, lhs[2] || rhs);
}
inline CHARM_SYCL_INLINE range<3> operator<(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] < rhs[0], lhs[1] < rhs[1], lhs[2] < rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator<(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs < rhs[0], lhs < rhs[1], lhs < rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator<(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] < rhs, lhs[1] < rhs, lhs[2] < rhs);
}
inline CHARM_SYCL_INLINE range<3> operator>(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] > rhs[0], lhs[1] > rhs[1], lhs[2] > rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator>(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs > rhs[0], lhs > rhs[1], lhs > rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator>(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] > rhs, lhs[1] > rhs, lhs[2] > rhs);
}
inline CHARM_SYCL_INLINE range<3> operator<=(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] <= rhs[0], lhs[1] <= rhs[1], lhs[2] <= rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator<=(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs <= rhs[0], lhs <= rhs[1], lhs <= rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator<=(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] <= rhs, lhs[1] <= rhs, lhs[2] <= rhs);
}
inline CHARM_SYCL_INLINE range<3> operator>=(range<3> const& lhs, range<3> const& rhs) {
    return range<3>(lhs[0] >= rhs[0], lhs[1] >= rhs[1], lhs[2] >= rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator>=(size_t lhs, range<3> const& rhs) {
    return range<3>(lhs >= rhs[0], lhs >= rhs[1], lhs >= rhs[2]);
}
inline CHARM_SYCL_INLINE range<3> operator>=(range<3> const& lhs, size_t rhs) {
    return range<3>(lhs[0] >= rhs, lhs[1] >= rhs, lhs[2] >= rhs);
}
inline CHARM_SYCL_INLINE range<3>& operator+=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] += rhs[0];
    lhs[1] += rhs[1];
    lhs[2] += rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator+=(range<3>& lhs, size_t rhs) {
    lhs[0] += rhs;
    lhs[1] += rhs;
    lhs[2] += rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator-=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] -= rhs[0];
    lhs[1] -= rhs[1];
    lhs[2] -= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator-=(range<3>& lhs, size_t rhs) {
    lhs[0] -= rhs;
    lhs[1] -= rhs;
    lhs[2] -= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator*=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] *= rhs[0];
    lhs[1] *= rhs[1];
    lhs[2] *= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator*=(range<3>& lhs, size_t rhs) {
    lhs[0] *= rhs;
    lhs[1] *= rhs;
    lhs[2] *= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator/=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] /= rhs[0];
    lhs[1] /= rhs[1];
    lhs[2] /= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator/=(range<3>& lhs, size_t rhs) {
    lhs[0] /= rhs;
    lhs[1] /= rhs;
    lhs[2] /= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator%=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] %= rhs[0];
    lhs[1] %= rhs[1];
    lhs[2] %= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator%=(range<3>& lhs, size_t rhs) {
    lhs[0] %= rhs;
    lhs[1] %= rhs;
    lhs[2] %= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator<<=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] <<= rhs[0];
    lhs[1] <<= rhs[1];
    lhs[2] <<= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator<<=(range<3>& lhs, size_t rhs) {
    lhs[0] <<= rhs;
    lhs[1] <<= rhs;
    lhs[2] <<= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator>>=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] >>= rhs[0];
    lhs[1] >>= rhs[1];
    lhs[2] >>= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator>>=(range<3>& lhs, size_t rhs) {
    lhs[0] >>= rhs;
    lhs[1] >>= rhs;
    lhs[2] >>= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator&=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] &= rhs[0];
    lhs[1] &= rhs[1];
    lhs[2] &= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator&=(range<3>& lhs, size_t rhs) {
    lhs[0] &= rhs;
    lhs[1] &= rhs;
    lhs[2] &= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator|=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] |= rhs[0];
    lhs[1] |= rhs[1];
    lhs[2] |= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator|=(range<3>& lhs, size_t rhs) {
    lhs[0] |= rhs;
    lhs[1] |= rhs;
    lhs[2] |= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator^=(range<3>& lhs, range<3> const& rhs) {
    lhs[0] ^= rhs[0];
    lhs[1] ^= rhs[1];
    lhs[2] ^= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE range<3>& operator^=(range<3>& lhs, size_t rhs) {
    lhs[0] ^= rhs;
    lhs[1] ^= rhs;
    lhs[2] ^= rhs;
    return lhs;
}
CHARM_SYCL_END_NAMESPACE
