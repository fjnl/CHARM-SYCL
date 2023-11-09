#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <sycl/sycl.hpp>
#include "common.hpp"

void run_benchmark(float const* __restrict a, float const* __restrict b, float* __restrict c,
                   size_t n, size_t n_loop, size_t n_chunk, uint64_t* runtime) {
    std::vector<size_t> offsets, sizes;
    split_vecadd(n, offsets, sizes, n_chunk);

    std::vector<sycl::buffer<float, 1>> buff_a;
    std::vector<sycl::buffer<float, 1>> buff_b;
    std::vector<sycl::buffer<float, 1>> buff_c;
    sycl::queue q;

    for (size_t i = 0; i < n_chunk; i++) {
        buff_a.push_back({a + offsets.at(i), sizes.at(i)});
        buff_b.push_back({b + offsets.at(i), sizes.at(i)});
        buff_c.push_back({c + offsets.at(i), sizes.at(i)});
    }

    for (int loop = 0; loop < n_loop; loop++) {
        auto const t_start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < n_chunk; i++) {
            q.submit([&](sycl::handler& h) {
                sycl::accessor<float, 1, sycl::access_mode::read> d_a(buff_a.at(i), h);
                sycl::accessor<float, 1, sycl::access_mode::read> d_b(buff_b.at(i), h);
                sycl::accessor<float, 1, sycl::access_mode::write> d_c(buff_c.at(i), h);
                h.parallel_for(buff_a.at(i).get_range(), [=](sycl::id<1> i) {
                    d_c[i] = d_a[i] + d_b[i];
                });
            });
        }
        q.wait();

        auto const t_end = std::chrono::high_resolution_clock::now();
        runtime[loop] =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
    }
}
