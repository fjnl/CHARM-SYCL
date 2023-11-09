#include "common.hpp"

struct data {
    int x;
    int y;
};

int f(data d) {
    return d.x + d.y;
}

TEST_CASE("struct4", "") {
    sycl::queue q;

    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                data x;
                x.x = 1;
                x.y = 2;
                xx[0] = f(x);
            });
        });

        ev.wait();
    }

    REQUIRE(result == 3);
}
