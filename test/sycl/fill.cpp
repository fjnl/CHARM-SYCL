#include "common.hpp"

TEMPLATE_TEST_CASE_SIG("fill", "", ((int D), D), 1, 2, 3) {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }

    sycl::queue q;
    std::vector<int> host(pow<D>(10), -1);
    int const fill_val = 12345;

    {
        sycl::buffer buffer(host.data(), make_range<D>(10));
        q.submit([&](sycl::handler& h) {
            h.fill(buffer.get_access(h), fill_val);
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq);
        REQUIRE(host.at(seq) == fill_val);
    });
}

TEMPLATE_TEST_CASE_SIG("fill partial", "", ((int D), D), 1, 2, 3) {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }

    sycl::queue q;
    std::vector<int> host(pow<D>(10), -1);
    int const fill_val = 12345;

    {
        sycl::buffer buffer(host.data(), make_range<D>(10));
        q.submit([&](sycl::handler& h) {
            h.fill(buffer.get_access(h, make_range<D>(5), make_id<D>()), fill_val);
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq);

        if ((1 <= i && i < 6) && (D < 2 || (2 <= j && j < 7)) && (D < 3 || (3 <= k && k < 8))) {
            REQUIRE(host.at(seq) == fill_val);
        } else {
            REQUIRE(host.at(seq) == -1);
        }
    });
}
