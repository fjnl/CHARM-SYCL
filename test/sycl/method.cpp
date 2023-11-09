#include "common.hpp"

struct data {
    int x;

    int f() const {
        return x + 1;
    }
};

TEST_CASE("method", "") {
    sycl::queue q;

    data result{100};
    {
        sycl::buffer<data, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<data, 1, sycl::access_mode::read_write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                xx[0].x = xx[0].f();
            });
        });

        ev.wait();
    }

    REQUIRE(result.x == 100 + 1);
}
