#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "pointer"_test = [&]() {
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    int v = 0;
                    int* p = &v;
                    v += 1;
                    *p += 1;
                    xx[0] = v;
                });
            });
        }

        expect(result == 2_i);
    };

    "pointer2"_test = [&]() {
        sycl::queue q;
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    int v = 0;
                    int* p = &v;
                    *p = 123;
                    xx[0] = v;
                });
            });
        }

        expect(result == 123_i);
    };

    "pointer3"_test = [&]() {
        sycl::queue q;
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    int v = 0;
                    int* const p = &v;
                    *p = 123;
                    xx[0] = v;
                });
            });
        }

        expect(result == 123_i);
    };

    "pointer4"_test = [&]() {
        sycl::queue q;
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    int v = 0;
                    int* const p = &v;
                    *p = 123;
                    xx[0] = v;
                });
            });
        }

        expect(result == 123_i);
    };

    return 0;
}
