#include "common.hpp"

TEST_CASE("mul", "") {
    sycl::queue q;

    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                xx[0] = 2 * 3;
            });
        });

        ev.wait();
    }

    REQUIRE(result == 6);
}

TEST_CASE("mul_eq", "") {
    sycl::queue q;

    int result = -1;
    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int temp = 10;
                temp *= 2;
                xx[0] = temp;
            });
        });

        ev.wait();
    }

    REQUIRE(result == 20);
}
