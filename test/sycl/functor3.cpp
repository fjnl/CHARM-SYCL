#include "common.hpp"

template <class T, T Value>
struct kernel {
    explicit kernel(sycl::accessor<T, 1, sycl::access_mode::write> const& x) : x_(x) {}

    void operator()(sycl::id<1> const&) const {
        x_[0] += Value;
    }

    sycl::accessor<T, 1, sycl::access_mode::write> x_;
};

TEST_CASE("functor3", "") {
    sycl::queue q;

    int result = -1;
    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), kernel<int, 10>(xx));
        });

        ev.wait();

        auto ev2 = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), kernel<int, 20>(xx));
        });

        ev2.wait();
    }

    REQUIRE(result == -1 + 10 + 20);
}
