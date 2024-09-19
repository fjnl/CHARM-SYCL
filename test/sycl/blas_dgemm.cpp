#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "common.hpp"

using sycl::blas::trans;

template <bool, class T>
struct get_value_type {
    using type = T;
};

template <class T>
struct get_value_type<false, T> {
    using type = typename T::value_type;
};

template <class T>
auto make(typename get_value_type<std::is_scalar_v<T>, T>::type val) {
    if constexpr (std::is_scalar_v<T>) {
        return val;
    } else {
        return T(val, -0.1 * val);
    }
}

template <class T>
void run(typename get_value_type<std::is_scalar_v<T>, T>::type tol) {
    // C = alpha op(A) . op(B) + beta C
    //  row x col
    // A: M x K
    // B: K x N
    // C: M x N

    int constexpr M = 3, N = 4, K = 5;
    std::vector<T> a(M * K), b(K * N), c(M * N);

    // col-major
#define A_(row, col) (a.at((col) * M + (row)))
#define B_(row, col) (b.at((col) * K + (row)))
#define C_(row, col) (c.at((col) * M + (row)))

    for (int row = 0; row < M; ++row) {
        for (int col = 0; col < K; ++col) {
            A_(row, col) = make<T>(row * K + col);
        }
    }

    for (int row = 0; row < K; ++row) {
        for (int col = 0; col < N; ++col) {
            B_(row, col) = make<T>(row * N + col);
        }
    }

    for (int row = 0; row < M; ++row) {
        for (int col = 0; col < N; ++col) {
            C_(row, col) = make<T>(std::max(row, col));
        }
    }

    sycl::queue q;

    {
        sycl::buffer<T, 2> A(a.data(), {K, M}), B(b.data(), {N, K}), C(c.data(), {N, M});
        gemm(q, trans::N, trans::N, M, N, K, 1.0, A, B, -1.0, C);
    }

    for (int row = 0; row < M; ++row) {
        for (int col = 0; col < N; ++col) {
            auto expected = make<T>(-1.0 * std::max(row, col));
            for (int k = 0; k < K; ++k) {
                expected += A_(row, k) * B_(k, col);
            }

            auto const val = C_(row, col);

            if constexpr (std::is_scalar_v<T>) {
                REQUIRE_THAT(val, Catch::Matchers::WithinRel(expected, tol));
            } else {
                REQUIRE_THAT(std::abs(val),
                             Catch::Matchers::WithinRel(std::abs(expected), tol));
            }
        }
    }
}

TEST_CASE("SGEMM", "") {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }
    run<float>(1.0e-5);
}

TEST_CASE("DGEMM", "") {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }
    run<double>(1.0e-10);
}

TEST_CASE("CGEMM", "") {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }
    run<std::complex<float>>(1.0e-5);
}

TEST_CASE("ZGEMM", "") {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }
    run<std::complex<double>>(1.0e-10);
}
