#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "single_task"_test = [&](auto rand_val) {
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.single_task([=]() {
                    xx[0] = rand_val;
                });
            });
        }

        expect(eq(result, rand_val));
    } | random_values(3, 1, 10000);

    return 0;
}
