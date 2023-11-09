#pragma once

#include <stdexcept>
#include <catch2/catch_session.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <stdio.h>
#include <charm/sycl.hpp>

int main(int argc, char** argv) {
    try {
#ifndef NO_RETAIN_QUEUE
        sycl::queue dummy;
#endif
        return Catch::Session().run(argc, argv);
    } catch (std::exception const& e) {
        fprintf(stderr, "Test Error: an exception caught outside the test");
        fprintf(stderr, "%s\n", e.what());
        return 1;
    } catch (...) {
        fprintf(stderr, "Test Error: an exception caught outside the test");
        return 1;
    }
}

template <int D>
int pow(int x) {
    int r = 1;
    for (int d = 0; d < D; ++d) {
        r *= x;
    }
    return r;
}

template <int D>
sycl::range<D> make_range(size_t n) {
    if constexpr (D == 1) {
        return {n};
    } else if constexpr (D == 2) {
        return {n, n};
    } else {
        return {n, n, n};
    }
}

template <int D>
sycl::range<D> make_range(int r0, int r1, int r2) {
    if constexpr (D == 1) {
        return sycl::range(r0);
    } else if constexpr (D == 2) {
        return sycl::range(r0, r1);
    } else {
        return sycl::range(r0, r1, r2);
    }
}

template <int D>
sycl::id<D> make_id() {
    if constexpr (D == 1) {
        return {1};
    } else if constexpr (D == 2) {
        return {1, 2};
    } else {
        return {1, 2, 3};
    }
}

template <int D>
sycl::id<D> make_id(size_t i0) {
    if constexpr (D == 1) {
        return {i0};
    } else if constexpr (D == 2) {
        return {i0, i0};
    } else {
        return {i0, i0, i0};
    }
}

template <int D, class F>
void iterate(sycl::range<D> const& r, F const& f) {
    size_t seq = 0;

    if constexpr (D == 1) {
        for (size_t i = 0; i < r[0]; i++) {
            f(i, 0, 0, seq);
            seq++;
        }
    } else if constexpr (D == 2) {
        for (size_t i = 0; i < r[0]; i++)
            for (size_t j = 0; j < r[1]; j++) {
                f(i, j, 0, seq);
                seq++;
            }
    } else {
        for (size_t i = 0; i < r[0]; i++)
            for (size_t j = 0; j < r[1]; j++)
                for (size_t k = 0; k < r[2]; k++) {
                    f(i, j, k, seq);
                    seq++;
                }
    }
}
