#include "ut_common.hpp"

struct data {
    int x;

    int f() const {
        return x + 1;
    }
};

int main() {
    sycl::queue q;

    "method"_test = [&]() {
        data result{100};
        {
            sycl::buffer<data, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<data, 1, sycl::access_mode::read_write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    xx[0].x = xx[0].f();
                });
            });

            ev.wait();
        }

        expect(result.x == 101_i);
    };

    return 0;
}
