#include "ut_common.hpp"

struct kernel {
    explicit kernel(sycl::accessor<int, 1, sycl::access_mode::write> const& x) : x_(x) {}

    void operator()(sycl::id<1> const&) const {
        x_[0] = 100;
    }

    sycl::accessor<int, 1, sycl::access_mode::write> x_;
};

int main() {
    sycl::queue q;

    "functor"_test = [&]() {
        int result = -1;
        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                kernel fn(xx);

                h.parallel_for(sycl::range(1), fn);
            });

            ev.wait();
        }

        expect(result == 100_i);
    };

    return 0;
}
