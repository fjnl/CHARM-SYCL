
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
struct id<3> {
    inline id();
    inline id(size_t dim0, size_t dim1, size_t dim2);
    inline id(range<3> const&);
    inline id(item<3> const&);
    inline CHARM_SYCL_INLINE size_t size() const;
    inline CHARM_SYCL_INLINE size_t get(size_t dimension) const;
    inline CHARM_SYCL_INLINE size_t& operator[](size_t dimension);
    inline CHARM_SYCL_INLINE size_t const& operator[](size_t dimension) const;
    friend inline CHARM_SYCL_INLINE bool operator==(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE bool operator!=(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator+(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator+(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator+(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator-(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator-(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator-(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator*(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator*(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator*(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator/(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator/(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator/(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator%(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator%(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator%(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<<(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<<(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<<(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>>(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>>(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>>(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator&(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator&(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator&(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator|(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator|(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator|(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator^(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator^(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator^(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator&&(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator&&(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator&&(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator||(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator||(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator||(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<=(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<=(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator<=(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>=(id<3> const& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>=(size_t lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3> operator>=(id<3> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator+=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator+=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator-=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator-=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator*=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator*=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator/=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator/=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator%=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator%=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator<<=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator<<=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator>>=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator>>=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator&=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator&=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator|=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator|=(id<3>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator^=(id<3>& lhs, id<3> const& rhs);
    friend inline CHARM_SYCL_INLINE id<3>& operator^=(id<3>& lhs, size_t rhs);

private:
    size_t id_[3];
};
id(size_t, size_t, size_t) -> id<3>;
CHARM_SYCL_END_NAMESPACE
