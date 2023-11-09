#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include "common.hpp"

void f(sycl::queue& q, sycl::buffer<int, 1>& buffer, int const& val) {
    q.submit([&](sycl::handler& h) {
        sycl::accessor<int, 1, sycl::access_mode::write> xx(buffer, h);
        h.single_task([=] {
            xx[0] = val;
        });
    });
}

TEST_CASE("caputure4", "") {
    sycl::queue q;

    int result = -1;
    int const rand_val =
        GENERATE(Catch::Generators::take(3, Catch::Generators::random(1, 10000)));

    {
        sycl::buffer<int, 1> x(&result, {1});

        f(q, x, rand_val);
    }

    REQUIRE(result == rand_val);
}
