#include "common.hpp"

struct kernel {
    explicit kernel(sycl::accessor<int, 1, sycl::access_mode::write> const& x) : x_(x) {}

    void operator()(sycl::id<1> const&) const {
        x_[0] = 100;
    }

    sycl::accessor<int, 1, sycl::access_mode::write> x_;
};

TEST_CASE("functor2", "") {
    sycl::queue q;

    int result = -1;
    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), kernel(xx));
        });

        ev.wait();
    }

    REQUIRE(result == 100);
}
