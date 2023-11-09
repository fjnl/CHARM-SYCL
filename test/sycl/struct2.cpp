#include "common.hpp"

struct data {
    int x = 1;
    int y = 2;
};

TEST_CASE("struct2", "") {
    sycl::queue q;

    data result;
    result.x = -1;
    result.y = -1;

    {
        sycl::buffer<data, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<data, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                data x;
                xx[0] = x;
            });
        });

        ev.wait();
    }

    REQUIRE(result.x == 1);
    REQUIRE(result.y == 2);
}
