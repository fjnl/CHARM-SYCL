#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "sqrt"_test = [&]() {
        float result = -1;
        {
            sycl::buffer<float, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<float, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    xx[0] = sycl::sqrt(81.0f);
                });
            });

            ev.wait();
        }

        expect(result == 9.0_f);
    };

    return 0;
}
