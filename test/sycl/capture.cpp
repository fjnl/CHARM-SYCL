#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "capture"_test = [&](auto rand_val) {
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    xx[0] = rand_val;
                });
            });
        }

        expect(eq(result, rand_val));
    } | random_values(3, 1, 10000);

    return 0;
}
