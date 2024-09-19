
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
struct id<2> {
    inline id();
    inline id(size_t dim0, size_t dim1);
    inline id(range<2> const&);
    inline id(item<2> const&);
    inline CHARM_SYCL_INLINE size_t size() const;
    inline CHARM_SYCL_INLINE size_t get(size_t dimension) const;
    inline CHARM_SYCL_INLINE size_t& operator[](size_t dimension);
    inline CHARM_SYCL_INLINE size_t const& operator[](size_t dimension) const;
    friend inline CHARM_SYCL_INLINE bool operator==(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE bool operator!=(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator+(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator+(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator+(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator-(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator-(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator-(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator*(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator*(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator*(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator/(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator/(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator/(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator%(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator%(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator%(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<<(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<<(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<<(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>>(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>>(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>>(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator&(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator&(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator&(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator|(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator|(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator|(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator^(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator^(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator^(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator&&(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator&&(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator&&(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator||(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator||(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator||(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<=(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<=(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator<=(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>=(id<2> const& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>=(size_t lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2> operator>=(id<2> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator+=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator+=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator-=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator-=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator*=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator*=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator/=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator/=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator%=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator%=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator<<=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator<<=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator>>=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator>>=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator&=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator&=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator|=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator|=(id<2>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator^=(id<2>& lhs, id<2> const& rhs);
    friend inline CHARM_SYCL_INLINE id<2>& operator^=(id<2>& lhs, size_t rhs);

private:
    size_t id_[2];
};
id(size_t, size_t) -> id<2>;
CHARM_SYCL_END_NAMESPACE
