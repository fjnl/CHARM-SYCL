#include <type_traits>
#include "ut_common.hpp"

template <class>
struct get_dim;

template <int D>
struct get_dim<sycl::id<D>> : std::integral_constant<int, D> {};

template <class Type>
void run_tests() {
    "id"_test = [] {
        auto constexpr N = get_dim<Type>::value;

        Type x;
        expect(x.size() == 0_i);

        if constexpr (N == 1) {
            expect(std::is_convertible_v<Type, size_t>);
        } else {
            expect(!std::is_convertible_v<Type, size_t>);
        }

        if constexpr (N == 1) {
            auto const x = sycl::range<N>(100);
            auto const y = Type(x);
            expect(eq(x.size(), y.size()));
        } else if constexpr (N == 2) {
            auto const x = sycl::range<N>(100, 200);
            auto const y = Type(x);
            expect(eq(x.size(), y.size()));
        } else {
            auto const x = sycl::range<N>(100, 200, 300);
            auto const y = Type(x);
            expect(eq(x.size(), y.size()));
        }
    };
}

int main() {
    run_tests<sycl::id<1>>();
    run_tests<sycl::id<2>>();
    run_tests<sycl::id<3>>();
    return 0;
}
