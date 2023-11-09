#include <chrono>
#include <cstdint>
#include <cstdlib>

void run_benchmark(float const* __restrict a, float const* __restrict b, float* __restrict c,
                   size_t n, size_t n_loop, size_t, uint64_t* runtime) {
#pragma acc enter data copyin(a[0 : n]) copyin(b[0 : n]) create(c[0 : n])

    for (int loop = 0; loop < n_loop; loop++) {
        auto const t_start = std::chrono::high_resolution_clock::now();

#pragma acc data present(a[0 : n]) present(b[0 : n]) present(c[0 : n])
#pragma acc kernels
        for (size_t i = 0; i < n; i++) {
            c[i] = a[i] + b[i];
        }

        auto const t_end = std::chrono::high_resolution_clock::now();
        runtime[loop] =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
    }

#pragma acc exit data copyout(c[0 : n])
}
