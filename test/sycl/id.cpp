#include <type_traits>
#include <catch2/catch_template_test_macros.hpp>
#include "common.hpp"

template <class>
struct get_dim;

template <int D>
struct get_dim<sycl::id<D>> : std::integral_constant<int, D> {};

TEMPLATE_TEST_CASE("id", "[id][sycl]", sycl::id<1>, sycl::id<2>, sycl::id<3>) {
    auto constexpr N = get_dim<TestType>::value;

    SECTION("default constructor") {
        TestType x;
        REQUIRE(x.size() == 0);
    }

    SECTION("operator size_t") {
        if constexpr (N == 1) {
            STATIC_REQUIRE(std::is_convertible_v<TestType, size_t>);
        } else {
            STATIC_REQUIRE(!std::is_convertible_v<TestType, size_t>);
        }
    }

    SECTION("constructor(range)") {
        if constexpr (N == 1) {
            auto const x = sycl::range<N>(100);
            auto const y = TestType(x);
            REQUIRE(x.size() == y.size());
        } else if constexpr (N == 2) {
            auto const x = sycl::range<N>(100, 200);
            auto const y = TestType(x);
            REQUIRE(x.size() == y.size());
        } else {
            auto const x = sycl::range<N>(100, 200, 300);
            auto const y = TestType(x);
            REQUIRE(x.size() == y.size());
        }
    }
}
