#include "common.hpp"

TEMPLATE_TEST_CASE_SIG("copy", "", ((int D), D), 1, 2, 3) {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }

    sycl::queue q;
    std::vector<int> h_src(pow<D>(10), -1);
    std::vector<int> h_dst(pow<D>(10), -1);

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        h_src.at(seq) = i * 100 + j * 10 + k;
    });

    {
        sycl::buffer src(h_src.data(), make_range<D>(10));
        sycl::buffer dst(h_dst.data(), make_range<D>(10));

        q.submit([&](sycl::handler& h) {
            h.copy(src.get_access(h), dst.get_access(h));
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq);

        REQUIRE(h_dst.at(seq) == i * 100 + j * 10 + k);
    });
}

TEMPLATE_TEST_CASE_SIG("copy partial", "", ((int D), D), 1, 2, 3) {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }

    sycl::queue q;
    std::vector<int> h_src(pow<D>(10), -1);
    std::vector<int> h_dst(pow<D>(10), -1);

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        h_src.at(seq) = i * 100 + j * 10 + k;
    });

    {
        sycl::buffer src(h_src.data(), make_range<D>(10));
        sycl::buffer dst(h_dst.data(), make_range<D>(10));

        q.submit([&](sycl::handler& h) {
            h.copy(src.get_access(h, make_range<D>(5), make_id<D>()),
                   dst.get_access(h, make_range<D>(5), make_id<D>()));
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq);

        if ((1 <= i && i < 6) && (D < 2 || (2 <= j && j < 7)) && (D < 3 || (3 <= k && k < 8))) {
            REQUIRE(h_dst.at(seq) == i * 100 + j * 10 + k);
        } else {
            REQUIRE(h_dst.at(seq) == -1);
        }
    });
}

TEMPLATE_TEST_CASE_SIG("copy partial 2", "", ((int D), D), 1, 2, 3) {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }

    sycl::queue q;
    std::vector<int> h_src(pow<D>(10), -1);
    std::vector<int> h_dst(pow<D>(10), -1);

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        h_src.at(seq) = i * 100 + j * 10 + k;
    });

    {
        sycl::buffer src(h_src.data(), make_range<D>(10));
        sycl::buffer dst(h_dst.data(), make_range<D>(10));

        q.submit([&](sycl::handler& h) {
            h.copy(src.get_access(h, make_range<D>(5)),
                   dst.get_access(h, make_range<D>(5), make_id<D>()));
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq);

        if ((1 <= i && i < 6) && (D < 2 || (2 <= j && j < 7)) && (D < 3 || (3 <= k && k < 8))) {
            REQUIRE(h_dst.at(seq) ==
                    (i - 1) * 100 + (D < 2 ? 0 : j - 2) * 10 + (D < 3 ? 0 : k - 3));
        } else {
            REQUIRE(h_dst.at(seq) == -1);
        }
    });
}

TEMPLATE_TEST_CASE_SIG("copy partial 3", "", ((int D), D), 1, 2, 3) {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }

    sycl::queue q;
    std::vector<int> h_src(pow<D>(10), -1);
    std::vector<int> h_dst(pow<D>(10), -1);

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        h_src.at(seq) = i * 100 + j * 10 + k;
    });

    {
        sycl::buffer src(h_src.data(), make_range<D>(10));
        sycl::buffer dst(h_dst.data(), make_range<D>(10));

        q.submit([&](sycl::handler& h) {
            h.copy(src.get_access(h, make_range<D>(5), make_id<D>()),
                   dst.get_access(h, make_range<D>(5)));
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq);

        if (i < 5 && j < 5 && k < 5) {
            REQUIRE(h_dst.at(seq) ==
                    (i + 1) * 100 + (D < 2 ? 0 : j + 2) * 10 + (D < 3 ? 0 : k + 3));
        } else {
            REQUIRE(h_dst.at(seq) == -1);
        }
    });
}
