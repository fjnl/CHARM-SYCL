
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
struct id<1> {
    inline id();
    inline id(size_t dim0);
    inline id(range<1> const&);
    inline id(item<1> const&);
    inline operator size_t() const;
    inline CHARM_SYCL_INLINE size_t size() const;
    inline CHARM_SYCL_INLINE size_t get(size_t dimension) const;
    inline CHARM_SYCL_INLINE size_t& operator[](size_t dimension);
    inline CHARM_SYCL_INLINE size_t const& operator[](size_t dimension) const;
    friend inline CHARM_SYCL_INLINE bool operator==(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE bool operator!=(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator+(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator+(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator+(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator-(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator-(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator-(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator*(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator*(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator*(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator/(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator/(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator/(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator%(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator%(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator%(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<<(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<<(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<<(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>>(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>>(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>>(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator&(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator&(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator&(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator|(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator|(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator|(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator^(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator^(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator^(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator&&(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator&&(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator&&(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator||(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator||(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator||(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<=(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<=(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator<=(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>=(id<1> const& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>=(size_t lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1> operator>=(id<1> const& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator+=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator+=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator-=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator-=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator*=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator*=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator/=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator/=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator%=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator%=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator<<=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator<<=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator>>=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator>>=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator&=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator&=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator|=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator|=(id<1>& lhs, size_t rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator^=(id<1>& lhs, id<1> const& rhs);
    friend inline CHARM_SYCL_INLINE id<1>& operator^=(id<1>& lhs, size_t rhs);

private:
    size_t id_[1];
};
id(size_t) -> id<1>;
CHARM_SYCL_END_NAMESPACE
