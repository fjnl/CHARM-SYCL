#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include "common.hpp"

#define _(expr)                                                                     \
    ({                                                                              \
        auto const err__ = (expr);                                                  \
        if (err__ != cudaSuccess) {                                                 \
            show_msg_fmt("CUDA Error: %s: %s\n", #expr, cudaGetErrorString(err__)); \
            app_exit(1);                                                            \
        }                                                                           \
    })

__global__ void vecadd_kernel(float const* __restrict a, float const* __restrict b,
                              float* __restrict c, size_t n) {
    auto const i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

void run_benchmark(float const* __restrict a, float const* __restrict b, float* __restrict c,
                   size_t n, size_t n_loop, size_t, uint64_t* runtime) {
    float* d_a = nullptr;
    float* d_b = nullptr;
    float* d_c = nullptr;
    cudaStream_t stream;

    _(cudaMalloc(&d_a, n * sizeof(float)));
    _(cudaMalloc(&d_b, n * sizeof(float)));
    _(cudaMalloc(&d_c, n * sizeof(float)));
    _(cudaStreamCreate(&stream));

    _(cudaMemcpyAsync(d_a, a, n * sizeof(float), cudaMemcpyHostToDevice, stream));
    _(cudaMemcpyAsync(d_b, b, n * sizeof(float), cudaMemcpyHostToDevice, stream));
    _(cudaStreamSynchronize(stream));

    auto const nb = (n + 255) / 256;
    auto const nt = std::min<size_t>(n, 256);

    for (int loop = 0; loop < n_loop; loop++) {
        auto const t_start = std::chrono::high_resolution_clock::now();

        vecadd_kernel<<<nb, nt, 0, stream>>>(d_a, d_b, d_c, n);
        _(cudaStreamSynchronize(stream));

        auto const t_end = std::chrono::high_resolution_clock::now();
        runtime[loop] =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
    }

    _(cudaMemcpyAsync(c, d_c, n * sizeof(float), cudaMemcpyDeviceToHost, stream));
    _(cudaStreamSynchronize(stream));

    _(cudaFree(d_a));
    _(cudaFree(d_b));
    _(cudaFree(d_c));
}
