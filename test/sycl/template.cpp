#include "ut_common.hpp"

template <class T, T I, T J>
static T template_test(sycl::queue& q) {
    T result = -1;

    {
        sycl::buffer<T, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<T, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                xx[0] = I + J;
            });
        });

        ev.wait();
    }

    return result;
}

int main() {
    sycl::queue q;

    "template"_test = [&]() {
        expect(template_test<int, 1, 2>(q) == 3_i);
        expect(template_test<long, 3, 4>(q) == 7_l);
    };

    return 0;
}
