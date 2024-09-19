#include "ut_common.hpp"

struct X {
    X() = default;

    int a = -1;
};

struct Y : X {
    Y() : X() {}

    int b = -1;
};

int main() {
    sycl::queue q;

    "inherit"_test = [&]() {
        Y result;

        {
            sycl::buffer<Y, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<Y, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    Y y;
                    y.a = 1;
                    y.b = 2;
                    xx[0] = y;
                });
            });

            ev.wait();
        }

        expect(result.a == 1_i);
        expect(result.b == 2_i);
    };

    return 0;
}
