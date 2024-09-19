#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <utility>
#include <charm/sycl/config.hpp>
#include "error.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

error::result<void> load_cuda();
error::result<void> load_hip();

namespace detail {

template <size_t N>
struct tag_name {
    constexpr tag_name(char const (&s)[N]) {
        for (size_t i = 0; i < N; i++) {
            name[i] = s[i];
        }
    }

    char name[N];
};

struct zero_init_t {
    constexpr inline friend bool operator==(zero_init_t const&, zero_init_t const&) {
        return true;
    }
    constexpr inline friend bool operator!=(zero_init_t const&, zero_init_t const&) {
        return false;
    }
};

inline constexpr auto zero_init = zero_init_t();

template <class T>
struct init_val {
    constexpr init_val(T v) : val(v) {}
    T val;

    constexpr inline friend bool operator==(init_val const&, zero_init_t const&) {
        return false;
    }
    constexpr inline friend bool operator!=(init_val const&, zero_init_t const&) {
        return true;
    }
};

template <class T, class U = T>
concept Addable = requires(T x, U y) { x + y; };

template <class T, class U = T>
concept Comparable = requires(U x, U y) {
    x == y;
    x != y;
};

template <class Owner, class T, auto Name, auto Init = zero_init>
struct tagged_t {
    using native = T;

    tagged_t()
        requires(Init != zero_init)
        : val_(Init.val) {}

    tagged_t()
        requires(Init == zero_init && std::is_pointer_v<T>)
        : val_(nullptr) {}

    tagged_t()
        requires(Init == zero_init && (std::is_integral_v<T> || std::is_floating_point_v<T>))
        : val_(0) {}

    tagged_t()
        requires(Init == zero_init && std::is_class_v<T> && std::is_trivially_copyable_v<T>)
    {
        memset(&val_, 0x00, sizeof(val_));
    }

    explicit constexpr tagged_t(T const& val) : val_(val) {}

    explicit operator bool() const {
        return static_cast<bool>(val_);
    }

    T get() const {
        return val_;
    }

    T* address() {
        return &val_;
    }

    T const* address() const {
        return &val_;
    }

    T& operator*() {
        return val_;
    }

    T const& operator*() const {
        return val_;
    }

    T* operator->() {
        return &val_;
    }

    T const* operator->() const {
        return &val_;
    }

    inline friend bool operator==(tagged_t const& lhs, tagged_t const& rhs)
        requires Comparable<T>
    {
        return lhs.val_ == rhs.val_;
    }

    inline friend bool operator!=(tagged_t const& lhs, tagged_t const& rhs)
        requires Comparable<T>
    {
        return lhs.val_ != rhs.val_;
    }

    inline friend tagged_t operator+(tagged_t const& lhs, tagged_t const& rhs)
        requires Addable<T>
    {
        return tagged_t(lhs.val_ + rhs.val_);
    }

    template <class U>
    inline friend tagged_t operator+(tagged_t const& lhs, U const& rhs)
        requires Addable<T, U>
    {
        return tagged_t(lhs.val_ + rhs);
    }

private:
    T val_;
};

template <class>
struct is_tagged : std::false_type {};

template <class Owner, class T, auto Name, auto Init>
struct is_tagged<tagged_t<Owner, T, Name, Init>> : std::true_type {};

template <class T>
concept AnyTag = is_tagged<std::remove_reference_t<std::remove_cvref_t<T>>>::value;

template <size_t Size, size_t Align>
struct record_type {
private:
    alignas(Align) std::byte data_[Size];
};

template <class Owner, class T, auto Name, auto Init, class U>
void wrap_(tagged_t<Owner, T, Name, Init>& out, U&& val) {
    out = tagged_t<Owner, T, Name, Init>(std::forward<U>(val));
}

template <class T, AnyTag U>
T wrap(U&& val) {
    T out;
    wrap_(out, std::forward<U>(val));
    return out;
}

template <class T, class U>
T wrap(U&& val) {
    return T(std::forward<U>(val));
}

template <AnyTag T>
auto unwrap(T const& val) {
    return val.get();
}

template <AnyTag T>
auto unwrap(T* val) {
    return val->address();
}

template <AnyTag T>
auto unwrap(T const* val) {
    return val->address();
}

template <class T>
    requires(!AnyTag<T>)
auto unwrap(T const& val) {
    return val;
}

}  // namespace detail

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
