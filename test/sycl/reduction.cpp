#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "reduction"_test = [&]() {
        "reduction single"_test = [&]() {
            double result = -999;

            {
                sycl::buffer<double> x(&result, 1);

                q.submit([&](sycl::handler& h) {
                    auto x_red = sycl::reduction(x, h, sycl::plus<>());

                    h.parallel_for(sycl::range(1023), x_red, [=](sycl::id<1> const&, auto& xx) {
                        xx += 1.0;
                    });
                });
            }

            expect(result == 1023.00000_d);
        };

        "reduction twice"_test = [&]() {
            double result = -999;

            {
                sycl::buffer<double> x(&result, 1);

                q.submit([&](sycl::handler& h) {
                    auto x_red = sycl::reduction(x, h, sycl::plus<>());

                    h.parallel_for(sycl::range(1023), x_red, [=](sycl::id<1> const&, auto& xx) {
                        xx += 1.0;
                    });
                });

                q.submit([&](sycl::handler& h) {
                    auto x_red = sycl::reduction(x, h, sycl::plus<>());

                    h.parallel_for(sycl::range(1023), x_red, [=](sycl::id<1> const&, auto& xx) {
                        xx += 1.0;
                    });
                });
            }

            expect(result == 1023.00000_d);
        };
    };

    return 0;
}
