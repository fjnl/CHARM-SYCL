#include "common.hpp"

struct X {
    X() = default;

    int a = -1;
};

struct Y : X {
    Y() : X() {}

    int b = -1;
};

TEST_CASE("inherit", "") {
    sycl::queue q;

    Y result;

    {
        sycl::buffer<Y, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<Y, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                Y y;
                y.a = 1;
                y.b = 2;
                xx[0] = y;
            });
        });

        ev.wait();
    }

    REQUIRE(result.a == 1);
    REQUIRE(result.b == 2);
}
