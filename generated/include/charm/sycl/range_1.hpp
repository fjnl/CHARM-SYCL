
#pragma once
#include <cstdlib>
CHARM_SYCL_BEGIN_NAMESPACE
template <int>
struct id;
template <int, bool>
struct item;
template <int>
struct range;
template <>
struct range<1> {
    inline range(size_t dim0);
    inline CHARM_SYCL_INLINE size_t size() const;
    inline CHARM_SYCL_INLINE size_t get(size_t dimension) const;
    inline CHARM_SYCL_INLINE size_t& operator[](size_t dimension);
    inline CHARM_SYCL_INLINE size_t const& operator[](size_t dimension) const;
    friend inline CHARM_SYCL_INLINE bool operator==(range<1> const& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE bool operator!=(range<1> const& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator+(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator+(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator+(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator-(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator-(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator-(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator*(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator*(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator*(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator/(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator/(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator/(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator%(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator%(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator%(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<<(range<1> const& lhs,
                                                        range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<<(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<<(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>>(range<1> const& lhs,
                                                        range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>>(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>>(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator&(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator&(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator&(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator|(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator|(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator|(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator^(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator^(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator^(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator&&(range<1> const& lhs,
                                                        range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator&&(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator&&(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator||(range<1> const& lhs,
                                                        range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator||(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator||(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>(range<1> const& lhs,
                                                       range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<=(range<1> const& lhs,
                                                        range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<=(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator<=(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>=(range<1> const& lhs,
                                                        range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>=(size_t lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1> operator>=(range<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator+=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator+=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator-=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator-=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator*=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator*=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator/=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator/=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator%=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator%=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator<<=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator<<=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator>>=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator>>=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator&=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator&=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator|=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator|=(range<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator^=(range<1>& lhs, range<1> const& rhs);
    friend inline CHARM_SYCL_INLINE range<1>& operator^=(range<1>& lhs, size_t rhs);

private:
    size_t range_[1];
};
range(size_t) -> range<1>;
CHARM_SYCL_END_NAMESPACE
