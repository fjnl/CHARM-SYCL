
#pragma once
#include <charm/sycl.hpp>
CHARM_SYCL_BEGIN_NAMESPACE
id<2>::id() : id(0, 0) {}
id<2>::id(size_t dim0, size_t dim1) {
    (*this)[0] = dim0;
    (*this)[1] = dim1;
}
id<2>::id(range<2> const& r) {
    (*this)[0] = r[0];
    (*this)[1] = r[1];
}
id<2>::id(item<2> const& i) {
    (*this)[0] = i[0];
    (*this)[1] = i[1];
}
size_t id<2>::size() const {
    return (*this)[0] * (*this)[1];
}
size_t id<2>::get(size_t dimension) const {
    return (*this)[dimension];
}
size_t& id<2>::operator[](size_t dimension) {
    return id_[dimension];
}
size_t const& id<2>::operator[](size_t dimension) const {
    return id_[dimension];
}
inline CHARM_SYCL_INLINE bool operator==(id<2> const& lhs, id<2> const& rhs) {
    return lhs[0] == rhs[0] && lhs[1] == rhs[1];
}
inline CHARM_SYCL_INLINE bool operator!=(id<2> const& lhs, id<2> const& rhs) {
    return lhs[0] != rhs[0] && lhs[1] != rhs[1];
}
inline CHARM_SYCL_INLINE id<2> operator+(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] + rhs[0], lhs[1] + rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator+(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs + rhs[0], lhs + rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator+(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] + rhs, lhs[1] + rhs);
}
inline CHARM_SYCL_INLINE id<2> operator-(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] - rhs[0], lhs[1] - rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator-(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs - rhs[0], lhs - rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator-(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] - rhs, lhs[1] - rhs);
}
inline CHARM_SYCL_INLINE id<2> operator*(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] * rhs[0], lhs[1] * rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator*(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs * rhs[0], lhs * rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator*(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] * rhs, lhs[1] * rhs);
}
inline CHARM_SYCL_INLINE id<2> operator/(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] / rhs[0], lhs[1] / rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator/(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs / rhs[0], lhs / rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator/(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] / rhs, lhs[1] / rhs);
}
inline CHARM_SYCL_INLINE id<2> operator%(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] % rhs[0], lhs[1] % rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator%(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs % rhs[0], lhs % rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator%(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] % rhs, lhs[1] % rhs);
}
inline CHARM_SYCL_INLINE id<2> operator<<(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] << rhs[0], lhs[1] << rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator<<(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs << rhs[0], lhs << rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator<<(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] << rhs, lhs[1] << rhs);
}
inline CHARM_SYCL_INLINE id<2> operator>>(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] >> rhs[0], lhs[1] >> rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator>>(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs >> rhs[0], lhs >> rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator>>(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] >> rhs, lhs[1] >> rhs);
}
inline CHARM_SYCL_INLINE id<2> operator&(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] & rhs[0], lhs[1] & rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator&(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs & rhs[0], lhs & rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator&(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] & rhs, lhs[1] & rhs);
}
inline CHARM_SYCL_INLINE id<2> operator|(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] | rhs[0], lhs[1] | rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator|(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs | rhs[0], lhs | rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator|(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] | rhs, lhs[1] | rhs);
}
inline CHARM_SYCL_INLINE id<2> operator^(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] ^ rhs[0], lhs[1] ^ rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator^(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs ^ rhs[0], lhs ^ rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator^(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] ^ rhs, lhs[1] ^ rhs);
}
inline CHARM_SYCL_INLINE id<2> operator&&(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] && rhs[0], lhs[1] && rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator&&(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs && rhs[0], lhs && rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator&&(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] && rhs, lhs[1] && rhs);
}
inline CHARM_SYCL_INLINE id<2> operator||(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] || rhs[0], lhs[1] || rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator||(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs || rhs[0], lhs || rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator||(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] || rhs, lhs[1] || rhs);
}
inline CHARM_SYCL_INLINE id<2> operator<(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] < rhs[0], lhs[1] < rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator<(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs < rhs[0], lhs < rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator<(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] < rhs, lhs[1] < rhs);
}
inline CHARM_SYCL_INLINE id<2> operator>(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] > rhs[0], lhs[1] > rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator>(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs > rhs[0], lhs > rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator>(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] > rhs, lhs[1] > rhs);
}
inline CHARM_SYCL_INLINE id<2> operator<=(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] <= rhs[0], lhs[1] <= rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator<=(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs <= rhs[0], lhs <= rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator<=(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] <= rhs, lhs[1] <= rhs);
}
inline CHARM_SYCL_INLINE id<2> operator>=(id<2> const& lhs, id<2> const& rhs) {
    return id<2>(lhs[0] >= rhs[0], lhs[1] >= rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator>=(size_t lhs, id<2> const& rhs) {
    return id<2>(lhs >= rhs[0], lhs >= rhs[1]);
}
inline CHARM_SYCL_INLINE id<2> operator>=(id<2> const& lhs, size_t rhs) {
    return id<2>(lhs[0] >= rhs, lhs[1] >= rhs);
}
inline CHARM_SYCL_INLINE id<2>& operator+=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] += rhs[0];
    lhs[1] += rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator+=(id<2>& lhs, size_t rhs) {
    lhs[0] += rhs;
    lhs[1] += rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator-=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] -= rhs[0];
    lhs[1] -= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator-=(id<2>& lhs, size_t rhs) {
    lhs[0] -= rhs;
    lhs[1] -= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator*=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] *= rhs[0];
    lhs[1] *= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator*=(id<2>& lhs, size_t rhs) {
    lhs[0] *= rhs;
    lhs[1] *= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator/=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] /= rhs[0];
    lhs[1] /= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator/=(id<2>& lhs, size_t rhs) {
    lhs[0] /= rhs;
    lhs[1] /= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator%=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] %= rhs[0];
    lhs[1] %= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator%=(id<2>& lhs, size_t rhs) {
    lhs[0] %= rhs;
    lhs[1] %= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator<<=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] <<= rhs[0];
    lhs[1] <<= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator<<=(id<2>& lhs, size_t rhs) {
    lhs[0] <<= rhs;
    lhs[1] <<= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator>>=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] >>= rhs[0];
    lhs[1] >>= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator>>=(id<2>& lhs, size_t rhs) {
    lhs[0] >>= rhs;
    lhs[1] >>= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator&=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] &= rhs[0];
    lhs[1] &= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator&=(id<2>& lhs, size_t rhs) {
    lhs[0] &= rhs;
    lhs[1] &= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator|=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] |= rhs[0];
    lhs[1] |= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator|=(id<2>& lhs, size_t rhs) {
    lhs[0] |= rhs;
    lhs[1] |= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator^=(id<2>& lhs, id<2> const& rhs) {
    lhs[0] ^= rhs[0];
    lhs[1] ^= rhs[1];
    return lhs;
}
inline CHARM_SYCL_INLINE id<2>& operator^=(id<2>& lhs, size_t rhs) {
    lhs[0] ^= rhs;
    lhs[1] ^= rhs;
    return lhs;
}
CHARM_SYCL_END_NAMESPACE
