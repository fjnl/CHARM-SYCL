#include "common.hpp"

inline int add(int x, int y) {
    return x + y;
}

inline int sub(int const& x, int const& y) {
    return x - y;
}

TEST_CASE("function/1", "") {
    sycl::queue q;

    int result = -1;
    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                xx[0] = add(1, 2);
            });
        });

        ev.wait();
    }

    REQUIRE(result == 3);
}

TEST_CASE("function/2", "") {
    sycl::queue q;

    int result = -1;
    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int nine = 9;
                xx[0] = sub(nine + 1, 1);
            });
        });

        ev.wait();
    }

    REQUIRE(result == 9);
}
