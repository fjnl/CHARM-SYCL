#include <cinttypes>
#include <limits>
#include "ut_common.hpp"

enum X { a = 100, b };

enum Y : uintmax_t { c, d = std::numeric_limits<uintmax_t>::max() };

int main() {
    sycl::queue q;
    "enum_1"_test = [&]() {
        unsigned long result = -1;

        {
            sycl::buffer<unsigned long, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                auto out = x.get_access(h);

                h.single_task([=] {
                    out[0] = X::a;
                });
            });
        }

        expect(eq(result, X::a));
    };

    "enum_2"_test = [&]() {
        unsigned long result = -1;

        {
            sycl::buffer<unsigned long, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                auto out = x.get_access(h);

                h.single_task([=] {
                    out[0] = X::b;
                });
            });
        }

        expect(eq(result, X::b));
    };

    "enum_3"_test = [&]() {
        unsigned long result = -1;

        {
            sycl::buffer<unsigned long, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                auto out = x.get_access(h);

                h.single_task([=] {
                    out[0] = Y::c;
                });
            });
        }

        expect(eq(result, Y::c));
    };

    "enum_4"_test = [&]() {
        unsigned long result = -1;

        {
            sycl::buffer<unsigned long, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                auto out = x.get_access(h);

                h.single_task([=] {
                    out[0] = Y::d;
                });
            });
        }

        expect(eq(result, Y::d));
    };

    return 0;
}
