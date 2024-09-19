#include "ut_common.hpp"

struct data {
    int x;
    int y;
};

int f1(data d) {
    return d.x + d.y;
}

int f2(data& d) {
    return d.x + d.y;
}

int f3(data const& d) {
    return d.x + d.y;
}

int main() {
    sycl::queue q;

    "struct4"_test = [&]() {
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    data x;
                    x.x = 1;
                    x.y = 2;
                    xx[0] = f1(x) + f2(x) + f3(x);
                });
            });

            ev.wait();
        }

        expect(result == 9_i);
    };

    return 0;
}
