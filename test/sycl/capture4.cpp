#include "ut_common.hpp"

void f(sycl::queue& q, sycl::buffer<int, 1>& buffer, int const& val) {
    q.submit([&](sycl::handler& h) {
        sycl::accessor<int, 1, sycl::access_mode::write> xx(buffer, h);
        h.single_task([=] {
            xx[0] = val;
        });
    });
}

int main() {
    sycl::queue q;

    "capture4"_test = [&](auto rand_val) {
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            f(q, x, rand_val);
        }

        expect(eq(result, rand_val));
    } | random_values(3, 1, 10000);

    return 0;
}
