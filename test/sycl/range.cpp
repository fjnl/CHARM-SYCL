#include <type_traits>
#include <catch2/catch_template_test_macros.hpp>
#include "common.hpp"

template <class>
struct get_dim;

template <int D>
struct get_dim<sycl::range<D>> : std::integral_constant<int, D> {};

template <class T>
auto data(size_t x = 0) {
    auto constexpr N = get_dim<T>::value;
    if constexpr (N == 1) {
        return T(1 + x);
    } else if constexpr (N == 2) {
        return T(1 + x, 22 + x);
    } else {
        return T(1 + x, 22 + x, 333 + x);
    }
}

#define REQUIRE_1(...) REQUIRE(__VA_ARGS__)

#define REQUIRE_2(...)        \
    if constexpr (N >= 2) {   \
        REQUIRE(__VA_ARGS__); \
    }

#define REQUIRE_3(...)        \
    if constexpr (N >= 3) {   \
        REQUIRE(__VA_ARGS__); \
    }

TEMPLATE_TEST_CASE("range", "[range][sycl]", sycl::range<1>, sycl::range<2>, sycl::range<3>) {
    auto constexpr N = get_dim<TestType>::value;

    SECTION("not default constructible") {
        STATIC_REQUIRE(!std::is_default_constructible_v<TestType>);
    }

    SECTION("constructor") {
        if constexpr (N == 1) {
            TestType(1);
        } else if constexpr (N == 2) {
            TestType(1, 2);
        } else {
            TestType(1, 2, 3);
        }
    }

    SECTION("operator []") {
        auto r = data<TestType>();
        REQUIRE(r[0] == 1);
        REQUIRE_2(r[1] == 22);
        REQUIRE_3(r[2] == 333);
    }

    SECTION("operator []=") {
        auto r = data<TestType>();
        r[0] = 100;
        if constexpr (N >= 2) {
            r[1] = 200;
        }
        if constexpr (N >= 3) {
            r[2] = 300;
        }
        REQUIRE(r[0] == 100);
        REQUIRE_2(r[1] == 200);
        REQUIRE_3(r[2] == 300);
    }

    SECTION("operator [] const") {
        auto const r = data<TestType>();
        REQUIRE(r[0] == 1);
        REQUIRE_2(r[1] == 22);
        REQUIRE_3(r[2] == 333);
    }

    SECTION("get()") {
        auto const r = data<TestType>();
        REQUIRE(r.get(0) == 1);
        REQUIRE_2(r.get(1) == 22);
        REQUIRE_3(r.get(2) == 333);
    }

    SECTION("operator ==") {
        REQUIRE(data<TestType>() == data<TestType>());
        REQUIRE_FALSE(data<TestType>() == data<TestType>(1));
    }

    SECTION("operator !=") {
        REQUIRE_FALSE(data<TestType>() != data<TestType>());
        REQUIRE(data<TestType>() != data<TestType>(1));
    }

    SECTION("copy constructor") {
        auto const x = data<TestType>();
        auto const y = TestType(x);
        REQUIRE(y == data<TestType>());
    }

    SECTION("copy assignment") {
        auto x = data<TestType>();
        auto const y = data<TestType>(1);
        x = y;
        REQUIRE(x == data<TestType>(1));
    }

    SECTION("move constructor") {
        auto const x = data<TestType>();
        auto const y = TestType(std::move(x));
        REQUIRE(y == data<TestType>());
    }

    SECTION("move assignment") {
        auto x = data<TestType>();
        auto const y = data<TestType>(1);
        x = std::move(y);
        REQUIRE(x == data<TestType>(1));
    }

    SECTION("size") {
        auto const x = data<TestType>();
        if constexpr (N == 1) {
            REQUIRE(x.size() == 1);
        } else if constexpr (N == 2) {
            REQUIRE(x.size() == 22);
        } else {
            REQUIRE(x.size() == 7326);
        }
    }

    SECTION("operator +(T, T)") {
        auto const x = data<TestType>();
        auto const y = data<TestType>();
        auto const z = x + y;
        REQUIRE(z.get(0) == 2);
        REQUIRE_2(z.get(1) == 44);
        REQUIRE_3(z.get(2) == 666);
    }

    SECTION("operator +(T, size_t)") {
        auto const x = data<TestType>();
        auto const z = x + 1;
        REQUIRE(z == data<TestType>(1));
    }

    SECTION("operator +(size_t, T)") {
        auto const x = data<TestType>();
        auto const z = 1 + x;
        REQUIRE(z == data<TestType>(1));
    }

    SECTION("operator +=(T)") {
        auto x = data<TestType>();
        auto const y = data<TestType>();
        x += y;
        REQUIRE(x.get(0) == 2);
        REQUIRE_2(x.get(1) == 44);
        REQUIRE_3(x.get(2) == 666);
    }

    SECTION("operator +=(size_t)") {
        auto x = data<TestType>();
        x += 1;
        REQUIRE(x == data<TestType>(1));
    }

    [[maybe_unused]] constexpr size_t x1 = 3;
    [[maybe_unused]] constexpr size_t x2 = 5;
    [[maybe_unused]] constexpr size_t x3 = 7;
    [[maybe_unused]] constexpr size_t y1 = 11;
    [[maybe_unused]] constexpr size_t y2 = 13;
    [[maybe_unused]] constexpr size_t y3 = 17;

#define BINARY_OP_SECTION(op)              \
    SECTION("operator" #op "(T, T)") {     \
        if constexpr (N == 1) {            \
            TestType const x(x1);          \
            TestType const y(y1);          \
            TestType const z = x op y;     \
            REQUIRE(z[0] == x[0] op y[0]); \
        } else if constexpr (N == 2) {     \
            TestType const x(x1, x2);      \
            TestType const y(y1, y2);      \
            TestType const z = x op y;     \
            REQUIRE(z[0] == x[0] op y[0]); \
            REQUIRE(z[1] == x[1] op y[1]); \
        } else {                           \
            TestType const x(x1, x2, x3);  \
            TestType const y(y1, y2, y3);  \
            TestType const z = x op y;     \
            REQUIRE(z[0] == x[0] op y[0]); \
            REQUIRE(z[1] == x[1] op y[1]); \
            REQUIRE(z[2] == x[2] op y[2]); \
        }                                  \
    }

    BINARY_OP_SECTION(+)
    BINARY_OP_SECTION(-)
    BINARY_OP_SECTION(*)
    BINARY_OP_SECTION(/)
}
