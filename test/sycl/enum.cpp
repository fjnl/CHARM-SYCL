#include <cinttypes>
#include <limits>
#include "common.hpp"

enum X { a = 100, b };

enum Y : uintmax_t { c, d = std::numeric_limits<uintmax_t>::max() };

TEST_CASE("enum 1 [X::a]", "") {
    sycl::queue q;

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

    REQUIRE(result == X::a);
}

TEST_CASE("enum 2 [X::b]", "") {
    sycl::queue q;

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

    REQUIRE(result == X::b);
}

TEST_CASE("enum 3 [Y::c]", "") {
    sycl::queue q;

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

    REQUIRE(result == Y::c);
}

TEST_CASE("enum 4 [Y::d]", "") {
    sycl::queue q;

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

    REQUIRE(result == Y::d);
}
