
#pragma once
#include <charm/sycl.hpp>
CHARM_SYCL_BEGIN_NAMESPACE
id<1>::id() : id(0) {}
id<1>::id(size_t dim0) {
    (*this)[0] = dim0;
}
id<1>::id(range<1> const& r) {
    (*this)[0] = r[0];
}
id<1>::id(item<1> const& i) {
    (*this)[0] = i[0];
}
id<1>::operator size_t() const {
    return (*this)[0];
}
size_t id<1>::size() const {
    return (*this)[0];
}
size_t id<1>::get(size_t dimension) const {
    return (*this)[dimension];
}
size_t& id<1>::operator[](size_t dimension) {
    return id_[dimension];
}
size_t const& id<1>::operator[](size_t dimension) const {
    return id_[dimension];
}
inline CHARM_SYCL_INLINE bool operator==(id<1> const& lhs, id<1> const& rhs) {
    return lhs[0] == rhs[0];
}
inline CHARM_SYCL_INLINE bool operator!=(id<1> const& lhs, id<1> const& rhs) {
    return lhs[0] != rhs[0];
}
inline CHARM_SYCL_INLINE id<1> operator+(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] + rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator+(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs + rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator+(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] + rhs);
}
inline CHARM_SYCL_INLINE id<1> operator-(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] - rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator-(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs - rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator-(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] - rhs);
}
inline CHARM_SYCL_INLINE id<1> operator*(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] * rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator*(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs * rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator*(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] * rhs);
}
inline CHARM_SYCL_INLINE id<1> operator/(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] / rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator/(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs / rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator/(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] / rhs);
}
inline CHARM_SYCL_INLINE id<1> operator%(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] % rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator%(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs % rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator%(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] % rhs);
}
inline CHARM_SYCL_INLINE id<1> operator<<(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] << rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator<<(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs << rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator<<(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] << rhs);
}
inline CHARM_SYCL_INLINE id<1> operator>>(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] >> rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator>>(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs >> rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator>>(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] >> rhs);
}
inline CHARM_SYCL_INLINE id<1> operator&(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] & rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator&(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs & rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator&(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] & rhs);
}
inline CHARM_SYCL_INLINE id<1> operator|(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] | rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator|(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs | rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator|(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] | rhs);
}
inline CHARM_SYCL_INLINE id<1> operator^(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] ^ rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator^(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs ^ rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator^(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] ^ rhs);
}
inline CHARM_SYCL_INLINE id<1> operator&&(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] && rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator&&(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs && rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator&&(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] && rhs);
}
inline CHARM_SYCL_INLINE id<1> operator||(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] || rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator||(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs || rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator||(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] || rhs);
}
inline CHARM_SYCL_INLINE id<1> operator<(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] < rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator<(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs < rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator<(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] < rhs);
}
inline CHARM_SYCL_INLINE id<1> operator>(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] > rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator>(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs > rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator>(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] > rhs);
}
inline CHARM_SYCL_INLINE id<1> operator<=(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] <= rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator<=(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs <= rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator<=(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] <= rhs);
}
inline CHARM_SYCL_INLINE id<1> operator>=(id<1> const& lhs, id<1> const& rhs) {
    return id<1>(lhs[0] >= rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator>=(size_t lhs, id<1> const& rhs) {
    return id<1>(lhs >= rhs[0]);
}
inline CHARM_SYCL_INLINE id<1> operator>=(id<1> const& lhs, size_t rhs) {
    return id<1>(lhs[0] >= rhs);
}
inline CHARM_SYCL_INLINE id<1>& operator+=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] += rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator+=(id<1>& lhs, size_t rhs) {
    lhs[0] += rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator-=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] -= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator-=(id<1>& lhs, size_t rhs) {
    lhs[0] -= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator*=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] *= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator*=(id<1>& lhs, size_t rhs) {
    lhs[0] *= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator/=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] /= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator/=(id<1>& lhs, size_t rhs) {
    lhs[0] /= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator%=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] %= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator%=(id<1>& lhs, size_t rhs) {
    lhs[0] %= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator<<=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] <<= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator<<=(id<1>& lhs, size_t rhs) {
    lhs[0] <<= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator>>=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] >>= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator>>=(id<1>& lhs, size_t rhs) {
    lhs[0] >>= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator&=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] &= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator&=(id<1>& lhs, size_t rhs) {
    lhs[0] &= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator|=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] |= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator|=(id<1>& lhs, size_t rhs) {
    lhs[0] |= rhs;
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator^=(id<1>& lhs, id<1> const& rhs) {
    lhs[0] ^= rhs[0];
    return lhs;
}
inline CHARM_SYCL_INLINE id<1>& operator^=(id<1>& lhs, size_t rhs) {
    lhs[0] ^= rhs;
    return lhs;
}
CHARM_SYCL_END_NAMESPACE
