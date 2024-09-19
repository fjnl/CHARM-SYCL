
#pragma once
#include <charm/sycl.hpp>
CHARM_SYCL_BEGIN_NAMESPACE
id<3>::id() : id(0, 0, 0) {}
id<3>::id(size_t dim0, size_t dim1, size_t dim2) {
    (*this)[0] = dim0;
    (*this)[1] = dim1;
    (*this)[2] = dim2;
}
id<3>::id(range<3> const& r) {
    (*this)[0] = r[0];
    (*this)[1] = r[1];
    (*this)[2] = r[2];
}
id<3>::id(item<3> const& i) {
    (*this)[0] = i[0];
    (*this)[1] = i[1];
    (*this)[2] = i[2];
}
size_t id<3>::size() const {
    return (*this)[0] * (*this)[1] * (*this)[2];
}
size_t id<3>::get(size_t dimension) const {
    return (*this)[dimension];
}
size_t& id<3>::operator[](size_t dimension) {
    return id_[dimension];
}
size_t const& id<3>::operator[](size_t dimension) const {
    return id_[dimension];
}
inline CHARM_SYCL_INLINE bool operator==(id<3> const& lhs, id<3> const& rhs) {
    return lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2];
}
inline CHARM_SYCL_INLINE bool operator!=(id<3> const& lhs, id<3> const& rhs) {
    return lhs[0] != rhs[0] && lhs[1] != rhs[1] && lhs[2] != rhs[2];
}
inline CHARM_SYCL_INLINE id<3> operator+(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator+(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs + rhs[0], lhs + rhs[1], lhs + rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator+(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] + rhs, lhs[1] + rhs, lhs[2] + rhs);
}
inline CHARM_SYCL_INLINE id<3> operator-(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator-(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs - rhs[0], lhs - rhs[1], lhs - rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator-(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] - rhs, lhs[1] - rhs, lhs[2] - rhs);
}
inline CHARM_SYCL_INLINE id<3> operator*(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator*(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs * rhs[0], lhs * rhs[1], lhs * rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator*(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] * rhs, lhs[1] * rhs, lhs[2] * rhs);
}
inline CHARM_SYCL_INLINE id<3> operator/(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] / rhs[0], lhs[1] / rhs[1], lhs[2] / rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator/(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs / rhs[0], lhs / rhs[1], lhs / rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator/(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] / rhs, lhs[1] / rhs, lhs[2] / rhs);
}
inline CHARM_SYCL_INLINE id<3> operator%(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] % rhs[0], lhs[1] % rhs[1], lhs[2] % rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator%(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs % rhs[0], lhs % rhs[1], lhs % rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator%(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] % rhs, lhs[1] % rhs, lhs[2] % rhs);
}
inline CHARM_SYCL_INLINE id<3> operator<<(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] << rhs[0], lhs[1] << rhs[1], lhs[2] << rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator<<(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs << rhs[0], lhs << rhs[1], lhs << rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator<<(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] << rhs, lhs[1] << rhs, lhs[2] << rhs);
}
inline CHARM_SYCL_INLINE id<3> operator>>(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] >> rhs[0], lhs[1] >> rhs[1], lhs[2] >> rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator>>(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs >> rhs[0], lhs >> rhs[1], lhs >> rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator>>(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] >> rhs, lhs[1] >> rhs, lhs[2] >> rhs);
}
inline CHARM_SYCL_INLINE id<3> operator&(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] & rhs[0], lhs[1] & rhs[1], lhs[2] & rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator&(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs & rhs[0], lhs & rhs[1], lhs & rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator&(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] & rhs, lhs[1] & rhs, lhs[2] & rhs);
}
inline CHARM_SYCL_INLINE id<3> operator|(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] | rhs[0], lhs[1] | rhs[1], lhs[2] | rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator|(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs | rhs[0], lhs | rhs[1], lhs | rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator|(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] | rhs, lhs[1] | rhs, lhs[2] | rhs);
}
inline CHARM_SYCL_INLINE id<3> operator^(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] ^ rhs[0], lhs[1] ^ rhs[1], lhs[2] ^ rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator^(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs ^ rhs[0], lhs ^ rhs[1], lhs ^ rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator^(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] ^ rhs, lhs[1] ^ rhs, lhs[2] ^ rhs);
}
inline CHARM_SYCL_INLINE id<3> operator&&(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] && rhs[0], lhs[1] && rhs[1], lhs[2] && rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator&&(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs && rhs[0], lhs && rhs[1], lhs && rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator&&(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] && rhs, lhs[1] && rhs, lhs[2] && rhs);
}
inline CHARM_SYCL_INLINE id<3> operator||(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] || rhs[0], lhs[1] || rhs[1], lhs[2] || rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator||(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs || rhs[0], lhs || rhs[1], lhs || rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator||(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] || rhs, lhs[1] || rhs, lhs[2] || rhs);
}
inline CHARM_SYCL_INLINE id<3> operator<(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] < rhs[0], lhs[1] < rhs[1], lhs[2] < rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator<(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs < rhs[0], lhs < rhs[1], lhs < rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator<(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] < rhs, lhs[1] < rhs, lhs[2] < rhs);
}
inline CHARM_SYCL_INLINE id<3> operator>(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] > rhs[0], lhs[1] > rhs[1], lhs[2] > rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator>(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs > rhs[0], lhs > rhs[1], lhs > rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator>(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] > rhs, lhs[1] > rhs, lhs[2] > rhs);
}
inline CHARM_SYCL_INLINE id<3> operator<=(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] <= rhs[0], lhs[1] <= rhs[1], lhs[2] <= rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator<=(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs <= rhs[0], lhs <= rhs[1], lhs <= rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator<=(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] <= rhs, lhs[1] <= rhs, lhs[2] <= rhs);
}
inline CHARM_SYCL_INLINE id<3> operator>=(id<3> const& lhs, id<3> const& rhs) {
    return id<3>(lhs[0] >= rhs[0], lhs[1] >= rhs[1], lhs[2] >= rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator>=(size_t lhs, id<3> const& rhs) {
    return id<3>(lhs >= rhs[0], lhs >= rhs[1], lhs >= rhs[2]);
}
inline CHARM_SYCL_INLINE id<3> operator>=(id<3> const& lhs, size_t rhs) {
    return id<3>(lhs[0] >= rhs, lhs[1] >= rhs, lhs[2] >= rhs);
}
inline CHARM_SYCL_INLINE id<3>& operator+=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] += rhs[0];
    lhs[1] += rhs[1];
    lhs[2] += rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator+=(id<3>& lhs, size_t rhs) {
    lhs[0] += rhs;
    lhs[1] += rhs;
    lhs[2] += rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator-=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] -= rhs[0];
    lhs[1] -= rhs[1];
    lhs[2] -= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator-=(id<3>& lhs, size_t rhs) {
    lhs[0] -= rhs;
    lhs[1] -= rhs;
    lhs[2] -= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator*=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] *= rhs[0];
    lhs[1] *= rhs[1];
    lhs[2] *= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator*=(id<3>& lhs, size_t rhs) {
    lhs[0] *= rhs;
    lhs[1] *= rhs;
    lhs[2] *= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator/=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] /= rhs[0];
    lhs[1] /= rhs[1];
    lhs[2] /= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator/=(id<3>& lhs, size_t rhs) {
    lhs[0] /= rhs;
    lhs[1] /= rhs;
    lhs[2] /= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator%=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] %= rhs[0];
    lhs[1] %= rhs[1];
    lhs[2] %= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator%=(id<3>& lhs, size_t rhs) {
    lhs[0] %= rhs;
    lhs[1] %= rhs;
    lhs[2] %= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator<<=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] <<= rhs[0];
    lhs[1] <<= rhs[1];
    lhs[2] <<= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator<<=(id<3>& lhs, size_t rhs) {
    lhs[0] <<= rhs;
    lhs[1] <<= rhs;
    lhs[2] <<= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator>>=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] >>= rhs[0];
    lhs[1] >>= rhs[1];
    lhs[2] >>= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator>>=(id<3>& lhs, size_t rhs) {
    lhs[0] >>= rhs;
    lhs[1] >>= rhs;
    lhs[2] >>= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator&=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] &= rhs[0];
    lhs[1] &= rhs[1];
    lhs[2] &= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator&=(id<3>& lhs, size_t rhs) {
    lhs[0] &= rhs;
    lhs[1] &= rhs;
    lhs[2] &= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator|=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] |= rhs[0];
    lhs[1] |= rhs[1];
    lhs[2] |= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator|=(id<3>& lhs, size_t rhs) {
    lhs[0] |= rhs;
    lhs[1] |= rhs;
    lhs[2] |= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator^=(id<3>& lhs, id<3> const& rhs) {
    lhs[0] ^= rhs[0];
    lhs[1] ^= rhs[1];
    lhs[2] ^= rhs[2];
    return lhs;
}
inline CHARM_SYCL_INLINE id<3>& operator^=(id<3>& lhs, size_t rhs) {
    lhs[0] ^= rhs;
    lhs[1] ^= rhs;
    lhs[2] ^= rhs;
    return lhs;
}
CHARM_SYCL_END_NAMESPACE
