#include "ut_common.hpp"

struct data {
    int x;
    int y;
};

int main() {
    sycl::queue q;

    "struct"_test = [&]() {
        data result;
        result.x = -1;
        result.y = -1;

        {
            sycl::buffer<data, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<data, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    data x;
                    x.x = 1;
                    x.y = 2;
                    xx[0] = x;
                });
            });

            ev.wait();
        }

        expect(result.x == 1_i);
        expect(result.y == 2_i);
    };

    return 0;
}
