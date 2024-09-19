#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "capture3"_test = [&](auto rand_val) {
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [xx, val = rand_val + 1](sycl::id<1> i) {
                    xx[i] = val;
                });
            });
        }

        expect(eq(result, rand_val + 1));
    } | random_values(3, 1, 10000);

    return 0;
}
