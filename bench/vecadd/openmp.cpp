#include <chrono>
#include <cstdint>
#include <cstdlib>

void run_benchmark(float const* __restrict a, float const* __restrict b, float* __restrict c,
                   size_t n, size_t n_loop, size_t, uint64_t* runtime) {
    for (size_t loop = 0; loop < n_loop; loop++) {
        auto const t_start = std::chrono::high_resolution_clock::now();

#pragma omp parallel for
        for (size_t i = 0; i < n; i++) {
            c[i] = a[i] + b[i];
        }

        auto const t_end = std::chrono::high_resolution_clock::now();
        runtime[loop] =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
    }
}
