#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include "common.hpp"

TEST_CASE("caputure", "") {
    sycl::queue q;

    int result = -1;
    int const rand_val =
        GENERATE(Catch::Generators::take(3, Catch::Generators::random(1, 10000)));

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                xx[0] = rand_val;
            });
        });

        ev.wait();
    }

    REQUIRE(result == rand_val);
}
