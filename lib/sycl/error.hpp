#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <charm/sycl/config.hpp>
#include "format.hpp"

CHARM_SYCL_BEGIN_NAMESPACE
namespace error {

struct [[nodiscard]] error_base {
    virtual ~error_base() = default;

    virtual std::string const& description() const noexcept = 0;
};

struct [[nodiscard]] error_with_msg : error_base {
    template <class... Args>
    explicit error_with_msg(format::format_string<Args...> fmt, Args&&... args)
        : msg_(format::format(fmt, std::forward<Args>(args)...)) {}

    std::string const& description() const noexcept override {
        return msg_;
    }

    template <class... Args>
    void append(format::format_string<Args...> fmt, Args&&... args) {
        if (!msg_.empty()) {
            msg_ += '\n';
        }
        msg_ += format::format(fmt, std::forward<Args>(args)...);
    }

private:
    std::string msg_;
};

using error = std::unique_ptr<error_base>;

template <class E>
struct [[nodiscard]] unexpected {
    explicit unexpected(E const& err)
        requires std::is_copy_constructible_v<E>
        : err_(err) {}

    explicit unexpected(E&& err)
        requires std::is_move_constructible_v<E>
        : err_(std::move(err)) {}

    E& error() & {
        return err_;
    }
    E&& error() && {
        return std::move(err_);
    }
    E const& error() const& {
        return err_;
    }

private:
    E err_;
};

template <class E, class... Args>
[[nodiscard]] auto make_error(Args&&... args) {
    return unexpected(std::make_unique<E>(std::forward<Args>(args)...));
}

template <class... Args>
[[nodiscard]] auto make_errorf(format::format_string<Args...> fmt, Args&&... args) {
    return unexpected(std::make_unique<error_with_msg>(fmt, std::forward<Args>(args)...));
}

namespace detail {

template <class E>
struct is_unexpected : std::false_type {};

template <class E>
struct is_unexpected<unexpected<E>> : std::true_type {};

template <class T>
inline constexpr bool is_unexpected_v = is_unexpected<T>::value;

template <class T, class E>
struct expected_storage {
    expected_storage()
        requires std::is_default_constructible_v<T>
    {
        value_from();
    }

    template <class U>
    [[nodiscard]] expected_storage(U const& val)
        requires(!is_unexpected_v<U> && std::is_constructible_v<T, U const&>)
    {
        value_from(val);
    }

    template <class U>
    [[nodiscard]] expected_storage(U&& val)
        requires(!is_unexpected_v<U> && std::is_constructible_v<T, U &&>)
    {
        value_from(std::move(val));
    }

    ~expected_storage()
        requires(!std::is_trivially_destructible_v<T> || !std::is_trivially_destructible_v<E>)
    {
        destruct();
    }

    ~expected_storage()
        requires(std::is_trivially_destructible_v<T> && std::is_trivially_destructible_v<E>)
    = default;

    T& value() & {
        return *val();
    }

    T&& value() && {
        return std::move(*val());
    }

    T const& value() const& {
        return *val();
    }

protected:
    void destruct() {
        if (has_err_) {
            err()->~E();
        } else {
            val()->~T();
        }
    }

    void value_from() {
        new (val()) T();
        has_err_ = false;
    }

    template <class U>
    void value_from(U const& u) {
        new (val()) T(u);
        has_err_ = false;
    }

    template <class U>
    void value_from(U&& u) {
        new (val()) T(std::move(u));
        has_err_ = false;
    }

    T* val() {
        return reinterpret_cast<T*>(u_.val);
    }

    E* err() {
        return reinterpret_cast<E*>(u_.err);
    }

private:
    union {
        alignas(T) std::byte val[sizeof(T)];
        alignas(E) std::byte err[sizeof(E)];
    } u_;

protected:
    bool has_err_;
};

template <class E>
struct expected_storage<void, E> {
    expected_storage() {
        value_from();
    }

    ~expected_storage()
        requires(!std::is_trivially_destructible_v<E>)
    {
        destruct();
    }

    ~expected_storage()
        requires(std::is_trivially_destructible_v<E>)
    = default;

    void value() & {}

    void value() && {}

    void value() const& {}

protected:
    void destruct() {
        if (has_err_) {
            err()->~E();
        }
    }

    void value_from() {
        has_err_ = false;
    }

    E* err() {
        return reinterpret_cast<E*>(u_.err);
    }

private:
    union {
        alignas(E) std::byte err[sizeof(E)];
    } u_;

protected:
    bool has_err_;
};

template <class T, class E>
struct expected_error : expected_storage<T, E> {
    using expected_storage<T, E>::expected_storage;

    template <class G>
    [[nodiscard]] expected_error(unexpected<G> const& err)
        requires std::is_constructible_v<E, G const&>
    {
        error_from(err.error());
    }

    template <class G>
    [[nodiscard]] expected_error(unexpected<G>&& err)
        requires std::is_constructible_v<E, G&&>
    {
        error_from(std::move(err).error());
    }

    E& error() & {
        return *this->err();
    }

    E&& error() && {
        return std::move(*this->err());
    }

    E const& error() const& {
        return *this->err();
    }

protected:
    template <class EE>
    void error_from(EE const& err) {
        new (this->err()) E(err);
        this->has_err_ = true;
    }

    template <class EE>
    void error_from(EE&& err) {
        new (this->err()) E(std::move(err));
        this->has_err_ = true;
    }
};

}  // namespace detail

template <class T, class E>
struct expected : detail::expected_error<T, E> {
    using detail::expected_error<T, E>::expected_error;

    expected(expected const& val) {
        if (val) {
            this->value_from(val.value());
        } else {
            this->error_from(val.error());
        }
    }

    expected(expected&& val) {
        if (val) {
            this->value_from(std::move(val).value());
        } else {
            this->error_from(std::move(val).error());
        }
    }

    template <class U, class G>
    expected(expected<U, G> const& val) {
        if (val) {
            this->value_from(val.value());
        } else {
            this->error_from(val.error());
        }
    }

    template <class U, class G>
    expected(expected<U, G>&& val) {
        if (val) {
            this->value_from(std::move(val).value());
        } else {
            this->error_from(std::move(val).error());
        }
    }

    explicit operator bool() const {
        return !this->has_err_;
    }

    expected& operator=(expected const& rhs) {
        static_assert(std::is_nothrow_destructible_v<T>);
        static_assert(std::is_nothrow_destructible_v<E>);
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        static_assert(std::is_nothrow_copy_constructible_v<E>);

        this->destruct();
        if (rhs) {
            this->value_from(rhs.value());
        } else {
            this->error_from(rhs.error());
        }
        return *this;
    }

    expected& operator=(expected&& rhs) {
        static_assert(std::is_nothrow_destructible_v<T>);
        static_assert(std::is_nothrow_destructible_v<E>);
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_move_constructible_v<E>);

        this->destruct();
        if (rhs) {
            this->value_from(std::move(rhs).value());
        } else {
            this->error_from(std::move(rhs).error());
        }
        return *this;
    }
};

template <class E>
struct expected<void, E> : detail::expected_error<void, E> {
    using detail::expected_error<void, E>::expected_error;

    expected(expected const& val) {
        if (val) {
            this->value_from();
        } else {
            this->error_from(val.error());
        }
    }

    expected(expected&& val) {
        if (val) {
            this->value_from();
        } else {
            this->error_from(std::move(val).error());
        }
    }

    template <class G>
    expected(expected<void, G> const& val) {
        if (val) {
            this->value_from();
        } else {
            this->error_from(val.error());
        }
    }

    template <class G>
    expected(expected<void, G>&& val) {
        if (val) {
            this->value_from();
        } else {
            this->error_from(std::move(val).error());
        }
    }

    explicit operator bool() const {
        return !this->has_err_;
    }

    expected& operator=(expected const& rhs) {
        static_assert(std::is_nothrow_destructible_v<E>);
        static_assert(std::is_nothrow_copy_constructible_v<E>);

        this->destruct();
        this->error_from(rhs.error());
        return *this;
    }

    expected& operator=(expected&& rhs) {
        static_assert(std::is_nothrow_destructible_v<E>);
        static_assert(std::is_nothrow_move_constructible_v<E>);

        this->destruct();
        this->error_from(std::move(rhs).error());
        return *this;
    }
};

template <class T>
using result = expected<T, error>;

#define CHECK_ERROR(expr)                                                     \
    ({                                                                        \
        auto&& _err(expr);                                                    \
        if (!_err) {                                                          \
            return CHARM_SYCL_NS::error::unexpected(std::move(_err).error()); \
        }                                                                     \
        std::move(_err).value();                                              \
    })

template <class T>
T& unwrap(result<T>& r) {
    if (!r) {
        format::print(std::cerr, "Fatal Error: {}\n", r.error()->description());
        std::exit(1);
    }
    return r.value();
}

template <class T>
T&& unwrap(result<T>&& r) {
    if (!r) {
        format::print(std::cerr, "Fatal Error: {}\n", std::move(r).error()->description());
        std::exit(1);
    }
    return std::move(r).value();
}

}  // namespace error
CHARM_SYCL_END_NAMESPACE
