#include <type_traits>
#include "ut_common.hpp"

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

#define EXPECT_1(...) expect(__VA_ARGS__)

#define EXPECT_2(...)        \
    if constexpr (N >= 2) {  \
        expect(__VA_ARGS__); \
    }

#define EXPECT_3(...)        \
    if constexpr (N >= 3) {  \
        expect(__VA_ARGS__); \
    }

template <class TestType>
void run_tests() {
    auto constexpr N = get_dim<TestType>::value;

    "range"_test = [&]() {
        "not default constructible"_test = [&]() {
            expect(!std::is_default_constructible_v<TestType>);
        };

        "constructor"_test = [&]() {
            if constexpr (N == 1) {
                TestType(1);
            } else if constexpr (N == 2) {
                TestType(1, 2);
            } else {
                TestType(1, 2, 3);
            }
            expect(true);
        };

        "operator []"_test = [&]() {
            auto r = data<TestType>();
            expect(r[0] == 1_i);
            EXPECT_2(r[1] == 22_i);
            EXPECT_3(r[2] == 333_i);
        };

        "operator []="_test = [&]() {
            auto r = data<TestType>();
            r[0] = 100;
            if constexpr (N >= 2) {
                r[1] = 200;
            }
            if constexpr (N >= 3) {
                r[2] = 300;
            }
            expect(r[0] == 100_i);
            EXPECT_2(r[1] == 200_i);
            EXPECT_3(r[2] == 300_i);
        };

        "operator [] const"_test = [&]() {
            auto const r = data<TestType>();
            expect(r[0] == 1_i);
            EXPECT_2(r[1] == 22_i);
            EXPECT_3(r[2] == 333_i);
        };

        "get()"_test = [&]() {
            auto const r = data<TestType>();
            expect(r.get(0) == 1_i);
            EXPECT_2(r.get(1) == 22_i);
            EXPECT_3(r.get(2) == 333_i);
        };

        "operator =="_test = [&]() {
            expect(eq(data<TestType>() == data<TestType>(), true));
            expect(eq(data<TestType>() == data<TestType>(1), false));
        };

        "operator !="_test = [&]() {
            expect(eq(data<TestType>() != data<TestType>(), false));
            expect(eq(data<TestType>() != data<TestType>(1), true));
        };

        "copy constructor"_test = [&]() {
            auto const x = data<TestType>();
            auto const y = TestType(x);
            expect(eq(y, data<TestType>()));
        };

        "copy assignment"_test = [&]() {
            auto x = data<TestType>();
            auto const y = data<TestType>(1);
            x = y;
            expect(eq(x, data<TestType>(1)));
        };

        "move constructor"_test = [&]() {
            auto const x = data<TestType>();
            auto const y = TestType(std::move(x));
            expect(eq(y, data<TestType>()));
        };

        "move assignment"_test = [&]() {
            auto x = data<TestType>();
            auto const y = data<TestType>(1);
            x = std::move(y);
            expect(eq(x, data<TestType>(1)));
        };

        "size"_test = [&]() {
            auto const x = data<TestType>();
            if constexpr (N == 1) {
                expect(x.size() == 1_i);
            } else if constexpr (N == 2) {
                expect(x.size() == 22_i);
            } else {
                expect(x.size() == 7326_i);
            }
        };

        "operator +(T, T)"_test = [&]() {
            auto const x = data<TestType>();
            auto const y = data<TestType>();
            auto const z = x + y;
            expect(z.get(0) == 2_i);
            EXPECT_2(z.get(1) == 44_i);
            EXPECT_3(z.get(2) == 666_i);
        };

        "operator +(T, size_t)"_test = [&]() {
            auto const x = data<TestType>();
            auto const z = x + 1;
            expect(eq(z, data<TestType>(1)));
        };

        "operator +(size_t, T)"_test = [&]() {
            auto const x = data<TestType>();
            auto const z = 1 + x;
            expect(eq(z, data<TestType>(1)));
        };

        "operator +=(T)"_test = [&]() {
            auto x = data<TestType>();
            auto const y = data<TestType>();
            x += y;
            expect(x.get(0) == 2_i);
            EXPECT_2(x.get(1) == 44_i);
            EXPECT_3(x.get(2) == 666_i);
        };

        "operator +=(size_t)"_test = [&]() {
            auto x = data<TestType>();
            x += 1;
            expect(eq(x, data<TestType>(1)));
        };

        [[maybe_unused]] constexpr size_t x1 = 3;
        [[maybe_unused]] constexpr size_t x2 = 5;
        [[maybe_unused]] constexpr size_t x3 = 7;
        [[maybe_unused]] constexpr size_t y1 = 11;
        [[maybe_unused]] constexpr size_t y2 = 13;
        [[maybe_unused]] constexpr size_t y3 = 17;

#define BINARY_OP_SECTION(op)               \
    "operator" #op "(T, T)"_test = [&]() {  \
        if constexpr (N == 1) {             \
            TestType const x(x1);           \
            TestType const y(y1);           \
            TestType const z = x op y;      \
            expect(eq(z[0], x[0] op y[0])); \
        } else if constexpr (N == 2) {      \
            TestType const x(x1, x2);       \
            TestType const y(y1, y2);       \
            TestType const z = x op y;      \
            expect(eq(z[0], x[0] op y[0])); \
            expect(eq(z[1], x[1] op y[1])); \
        } else {                            \
            TestType const x(x1, x2, x3);   \
            TestType const y(y1, y2, y3);   \
            TestType const z = x op y;      \
            expect(eq(z[0], x[0] op y[0])); \
            expect(eq(z[1], x[1] op y[1])); \
            expect(eq(z[2], x[2] op y[2])); \
        }                                   \
    };

        BINARY_OP_SECTION(+)
        BINARY_OP_SECTION(-)
        BINARY_OP_SECTION(*)
        BINARY_OP_SECTION(/)
    };
}

int main() {
    run_tests<sycl::range<1>>();
    run_tests<sycl::range<2>>();
    run_tests<sycl::range<3>>();

    return 0;
}
