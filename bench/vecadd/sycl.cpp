#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <sycl/sycl.hpp>
#include "common.hpp"

void run_benchmark(float const* __restrict a, float const* __restrict b, float* __restrict c,
                   size_t n, size_t n_loop, size_t, uint64_t* runtime) {
    sycl::buffer<float, 1> buff_a(a, {n});
    sycl::buffer<float, 1> buff_b(b, {n});
    sycl::buffer<float, 1> buff_c(c, {n});
    sycl::queue q;

    for (int loop = 0; loop < n_loop; loop++) {
        auto const t_start = std::chrono::high_resolution_clock::now();

        q.submit([&](sycl::handler& h) {
            sycl::accessor<float, 1, sycl::access_mode::read> d_a(buff_a, h);
            sycl::accessor<float, 1, sycl::access_mode::read> d_b(buff_b, h);
            sycl::accessor<float, 1, sycl::access_mode::write> d_c(buff_c, h);
            h.parallel_for(sycl::range{n}, [=](sycl::id<1> i) {
                d_c[i] = d_a[i] + d_b[i];
            });
        });
        q.wait();

        auto const t_end = std::chrono::high_resolution_clock::now();
        runtime[loop] =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_start).count();
    }
}
