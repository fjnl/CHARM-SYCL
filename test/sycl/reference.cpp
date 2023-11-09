#include "common.hpp"

TEST_CASE("reference", "") {
    sycl::queue q;
    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int idx = 0;
                int& idx_ref = idx;
                xx[idx_ref] = 12345;
            });
        });
    }

    REQUIRE(result == 12345);
}

TEST_CASE("reference2", "") {
    sycl::queue q;
    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int v = 0;
                int& p = v;
                v += 1;
                p += 1;
                xx[0] = v;
            });
        });
    }

    REQUIRE(result == 2);
}

TEST_CASE("reference3", "") {
    sycl::queue q;
    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int v = 0;
                int& p = v;
                p = 123;
                xx[0] = v;
            });
        });
    }

    REQUIRE(result == 123);
}
