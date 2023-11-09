#include "common.hpp"

TEMPLATE_TEST_CASE_SIG("copy2 1", "", ((int D), D), 1, 2, 3) {
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

        q.submit([&](sycl::handler& h) {
            h.copy(src.get_access(h), h_dst.data());
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq);

        REQUIRE(h_dst.at(seq) == i * 100 + j * 10 + k);
    });
}

TEMPLATE_TEST_CASE_SIG("copy2 2", "", ((int D), D), 1, 2, 3) {
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
        sycl::buffer dst(h_dst.data(), make_range<D>(10));

        q.submit([&](sycl::handler& h) {
            h.copy(h_src.data(), dst.get_access(h));
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq);

        REQUIRE(h_dst.at(seq) == i * 100 + j * 10 + k);
    });
}

TEMPLATE_TEST_CASE_SIG("copy2 3", "", ((int D), D), 1, 2, 3) {
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

        q.submit([&](sycl::handler& h) {
            h.copy(src.get_access(h, make_range<D>(5), make_id<D>()), h_dst.data());
        });
    }

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        size_t ii, jj = 0, kk = 0;

        if constexpr (D == 1) {
            ii = seq;
        } else if constexpr (D == 2) {
            ii = seq / 5;
            jj = seq % 5;
        } else {
            ii = seq / 25;
            jj = seq / 5 % 5;
            kk = seq % 5;
        }

        CAPTURE(i, j, k, seq, ii, jj, kk);

        if (seq < pow<D>(5)) {
            REQUIRE(h_dst.at(seq) ==
                    (ii + 1) * 100 + (jj + (D < 2 ? 0 : 2)) * 10 + (kk + (D < 3 ? 0 : 3)));
        } else {
            REQUIRE(h_dst.at(seq) == -1);
        }
    });
}

TEMPLATE_TEST_CASE_SIG("copy2 4", "", ((int D), D), 1, 2, 3) {
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
        sycl::buffer dst(h_dst.data(), make_range<D>(10));

        q.submit([&](sycl::handler& h) {
            h.copy(h_src.data(), dst.get_access(h, make_range<D>(5), make_id<D>()));
        });
    }

    size_t src_idx = 0;

    iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
        CAPTURE(i, j, k, seq, src_idx);

        if ((1 <= i && i < 6) && (D < 2 || (2 <= j && j < 7)) && (D < 3 || (3 <= k && k < 8))) {
            REQUIRE(h_dst.at(seq) == h_src.at(src_idx));
            src_idx++;
        } else {
            REQUIRE(h_dst.at(seq) == -1);
        }
    });
}
