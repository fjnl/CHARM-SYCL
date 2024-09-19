
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
struct range<3> {
    inline range(size_t dim0, size_t dim1, size_t dim2);
    inline CHARM_SYCL_INLINE size_t size() const;
    inline CHARM_SYCL_INLINE size_t get(size_t dimension) const;
    inline CHARM_SYCL_INLINE size_t& operator[](size_t dimension);
    inline CHARM_SYCL_INLINE size_t const& operator[](size_t dimension) const;
    friend inline CHARM_SYCL_INLINE bool operator==(range<3> const& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE bool operator!=(range<3> const& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator+(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator+(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator+(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator-(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator-(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator-(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator*(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator*(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator*(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator/(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator/(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator/(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator%(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator%(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator%(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<<(range<3> const& lhs,
                                                        range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<<(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<<(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>>(range<3> const& lhs,
                                                        range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>>(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>>(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator&(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator&(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator&(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator|(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator|(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator|(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator^(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator^(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator^(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator&&(range<3> const& lhs,
                                                        range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator&&(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator&&(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator||(range<3> const& lhs,
                                                        range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator||(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator||(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>(range<3> const& lhs,
                                                       range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<=(range<3> const& lhs,
                                                        range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<=(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator<=(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>=(range<3> const& lhs,
                                                        range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>=(size_t lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3> operator>=(range<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator+=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator+=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator-=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator-=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator*=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator*=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator/=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator/=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator%=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator%=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator<<=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator<<=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator>>=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator>>=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator&=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator&=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator|=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator|=(range<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator^=(range<3>& lhs, range<3> const& rhs);
    friend inline CHARM_SYCL_INLINE range<3>& operator^=(range<3>& lhs, size_t rhs);

private:
    size_t range_[3];
};
range(size_t, size_t, size_t) -> range<3>;
CHARM_SYCL_END_NAMESPACE
