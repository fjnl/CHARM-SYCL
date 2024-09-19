#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "for"_test = [&]() {
        int result = -1;
        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    int n = 0;
                    for (int i = 0; i < 10; i++) {
                        n += i;
                    }
                    xx[0] = n;
                });
            });

            ev.wait();
        }

        expect(eq(result, 0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9));
    };

    return 0;
}
