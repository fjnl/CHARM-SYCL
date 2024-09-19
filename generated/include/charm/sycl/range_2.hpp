
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
struct range<2> {
    inline range(size_t dim0, size_t dim1);
    inline CHARM_SYCL_INLINE size_t size() const;
    inline CHARM_SYCL_INLINE size_t get(size_t dimension) const;
    inline CHARM_SYCL_INLINE size_t& operator[](size_t dimension);
    inline CHARM_SYCL_INLINE size_t const& operator[](size_t dimension) const;
    friend inline CHARM_SYCL_INLINE bool operator==(range<2> const& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE bool operator!=(range<2> const& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator+(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator+(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator+(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator-(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator-(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator-(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator*(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator*(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator*(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator/(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator/(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator/(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator%(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator%(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator%(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<<(range<2> const& lhs,
                                                        range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<<(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<<(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>>(range<2> const& lhs,
                                                        range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>>(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>>(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator&(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator&(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator&(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator|(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator|(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator|(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator^(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator^(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator^(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator&&(range<2> const& lhs,
                                                        range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator&&(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator&&(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator||(range<2> const& lhs,
                                                        range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator||(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator||(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>(range<2> const& lhs,
                                                       range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<=(range<2> const& lhs,
                                                        range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<=(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator<=(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>=(range<2> const& lhs,
                                                        range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>=(size_t lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2> operator>=(range<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator+=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator+=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator-=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator-=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator*=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator*=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator/=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator/=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator%=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator%=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator<<=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator<<=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator>>=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator>>=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator&=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator&=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator|=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator|=(range<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator^=(range<2>& lhs, range<2> const& rhs);
    friend inline CHARM_SYCL_INLINE range<2>& operator^=(range<2>& lhs, size_t rhs);

private:
    size_t range_[2];
};
range(size_t, size_t) -> range<2>;
CHARM_SYCL_END_NAMESPACE
