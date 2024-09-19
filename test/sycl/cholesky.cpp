#include <random>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "common.hpp"

void cholesky(sycl::queue& q, std::vector<sycl::buffer<double, 2>>& A, size_t nt, size_t size) {
    /*
     * Based on Omni compiler's implementation:
     * https://github.com/omni-compiler/omni-compiler/blob/xmp2/libxmp2/cholesky_task/cholesky.c
     */
    using namespace sycl::blas;

    for (size_t k = 0; k < nt; k++) {
        potrf(q, uplo::L, size, A[k * nt + k]);

        for (size_t i = k + 1; i < nt; i++) {
            trsm(q, side::R, uplo::L, trans::T, diag::N, size, size, 1.0, A[k * nt + k],
                 A[i * nt + k]);
        }

        for (size_t i = k + 1; i < nt; i++) {
            for (size_t j = k + 1; j < i; j++) {
                gemm(q, trans::N, trans::T, size, size, size, -1.0, A[i * nt + k],
                     A[j * nt + k], 1.0, A[i * nt + j]);
            }
            syrk(q, uplo::L, trans::N, size, size, -1.0, A[i * nt + k], 1.0, A[i * nt + i]);
        }
    }
}

#define A(row, col) a.at((col) * N + (row))
#define B(row, col) b.at((col) * N + (row))
#define T(row, col) t.at((col) * T + (row))

TEST_CASE("cholesky", "") {
    if (::strcmp(::getenv("CHARM_SYCL_RTS"), "IRIS") == 0) {
        SKIP();
    }

    // size_t constexpr T = 16;
    // size_t constexpr NT = 2;
    size_t constexpr T = 16;
    size_t constexpr NT = 16;
    size_t constexpr N = NT * T;

    auto const seed = GENERATE(Catch::Generators::take(1, Catch::Generators::random(1, 10000)));

    std::vector<double> a(N * N);
    std::mt19937 g(seed);
    std::uniform_real_distribution<double> d(0, 1);

    for (size_t i = 0; i < N; i++) {
        for (size_t j = 0; j <= i; j++) {
            double v = d(g);
            if (i == j) {
                A(i, j) = v + 100.0;
            } else {
                A(i, j) = v;
                A(j, i) = v;
            }
        }
    }

    // printf("A=\n");
    // for (size_t i = 0; i < N; i++) {
    //     for (size_t j = 0; j < N; j++) {
    //         printf("%12.8f ", A(i, j));
    //     }
    //     printf("\n");
    // }
    // printf("\n");

    std::vector<sycl::buffer<double, 2>> buffers;
    std::vector<std::vector<double>> tiles(NT * NT);

    for (size_t ti = 0; ti < NT; ti++) {
        for (size_t tj = 0; tj < NT; tj++) {
            auto& t = tiles.at(ti * NT + tj);
            t.resize(T * T);

            for (size_t i = 0; i < T; i++) {
                for (size_t j = 0; j < T; j++) {
                    auto const ii = ti * T + i;
                    auto const jj = tj * T + j;
                    T(i, j) = A(ii, jj);
                }
            }

            buffers.push_back(sycl::buffer<double, 2>(t.data(), {T, T}));
        }
    }

    {
        sycl::queue q;
        cholesky(q, buffers, NT, T);
        buffers.clear();
    }

    std::vector<double> b(N * N);

    for (size_t ti = 0; ti < NT; ti++) {
        for (size_t tj = 0; tj < NT; tj++) {
            auto const& t = tiles.at(ti * NT + tj);

            for (size_t i = 0; i < T; i++) {
                for (size_t j = 0; j < T; j++) {
                    auto const ii = ti * T + i;
                    auto const jj = tj * T + j;
                    B(ii, jj) = T(i, j);
                }
            }
        }
    }

    // printf("B=\n");
    // for (size_t i = 0; i < N; i++) {
    //     for (size_t j = 0; j < N; j++) {
    //         printf("%12.8f ", B(i, j));
    //     }
    //     printf("\n");
    // }
    // printf("\n");

    // Check: A = L . L^T
    for (size_t i = 0; i < N; i++) {      // row
        for (size_t j = 0; j < N; j++) {  // col
            double s = 0;

            // s += b(i,k) * b(j,k)
            for (size_t k = 0; k <= std::min(i, j); k++) {
                double x, y;
                x = B(i, k);
                y = B(j, k);

                s += x * y;
            }

            auto const expected = A(i, j);
            // printf("i:%d j:%d  X:%e  Y:%e  ABS:%e  REL:%e  OK:%d\n", i, j, s, expected,
            //        std::abs(expected - s), std::abs((expected - s) / expected),
            //        std::abs((expected - s) / expected) < 1.0e-15);

            CAPTURE(i, j);
            REQUIRE_THAT(s, Catch::Matchers::WithinRel(expected, 1.0e-8));
        }
    }
}
