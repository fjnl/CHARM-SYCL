#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>
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
                   size_t n, size_t n_loop, size_t n_chunk, uint64_t* runtime) {
    int n_dev;
    _(cudaGetDeviceCount(&n_dev));

    std::vector<size_t> offsets, sizes;
    split_vecadd(n, offsets, sizes, n_chunk);

    std::vector<float*> d_a(n_chunk);
    std::vector<float*> d_b(n_chunk);
    std::vector<float*> d_c(n_chunk);
    std::vector<cudaStream_t> stream(n_chunk);

    for (size_t i = 0; i < n_chunk; i++) {
        auto const d_off = offsets.at(i);
        auto const d_size = sizes.at(i);
        auto const dev = i % n_dev;

        _(cudaSetDevice(dev));
        _(cudaMalloc(&d_a.at(i), d_size * sizeof(float)));
        _(cudaMalloc(&d_b.at(i), d_size * sizeof(float)));
        _(cudaMalloc(&d_c.at(i), d_size * sizeof(float)));

        _(cudaStreamCreate(&stream.at(i)));
        _(cudaMemcpyAsync(d_a.at(i), a + d_off, d_size * sizeof(float), cudaMemcpyHostToDevice,
                          stream.at(i)));
        _(cudaMemcpyAsync(d_b.at(i), b + d_off, d_size * sizeof(float), cudaMemcpyHostToDevice,
                          stream.at(i)));
    }
    for (int i = 0; i < n_chunk; i++) {
        _(cudaStreamSynchronize(stream.at(i)));
    }

    for (int loop = 0; loop < n_loop; loop++) {
        auto const t_start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < n_chunk; i++) {
            auto const d_size = sizes.at(i);
            auto const dev = i % n_dev;
            auto const nb = (d_size + 255) / 256;
            auto const nt = std::min<size_t>(d_size, 256);

            _(cudaSetDevice(dev));
            vecadd_kernel<<<nb, nt, 0, stream.at(i)>>>(d_a.at(i), d_b.at(i), d_c.at(i), d_size);
        }
        for (int i = 0; i < n_chunk; i++) {
            _(cudaStreamSynchronize(stream.at(i)));
        }

        auto const t_end = std::chrono::high_resolution_clock::now();
        runtime[loop] =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
    }

    for (size_t i = 0; i < n_chunk; i++) {
        auto const d_off = offsets.at(i);
        auto const d_size = sizes.at(i);

        _(cudaMemcpyAsync(c + d_off, d_c.at(i), d_size * sizeof(float), cudaMemcpyDeviceToHost,
                          stream.at(i)));
    }
    for (size_t i = 0; i < n_chunk; i++) {
        _(cudaStreamSynchronize(stream.at(i)));
        _(cudaStreamDestroy(stream.at(i)));
        _(cudaFree(d_a.at(i)));
        _(cudaFree(d_b.at(i)));
        _(cudaFree(d_c.at(i)));
    }
}
