#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include "common.hpp"

void run_benchmark(float const* __restrict a, float const* __restrict b, float* __restrict c,
                   size_t n, size_t loop, size_t chunk, uint64_t* runtime);

std::chrono::high_resolution_clock::time_point t_start;

namespace {

template <class G>
void initialize(float* __restrict a, float* __restrict b, size_t n, G g) {
    for (size_t i = 0; i < n; i++) {
        a[i] = (g() >> 8) * 0x1.0p-24;
    }
    for (size_t i = 0; i < n; i++) {
        b[i] = (g() >> 8) * 0x1.0p-24;
    }
}

void zerofill(float* __restrict x, size_t n) {
    for (size_t i = 0; i < n; i++) {
        x[i] = 0;
    }
}

void vecadd_verify(float const* __restrict a, float const* __restrict b, float* __restrict c,
                   size_t n) {
    for (size_t i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

bool run_verify(float const* __restrict expected, float const* __restrict result, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (expected[i] != result[i]) {
            return false;
        }
    }
    return true;
}

double compute_bw(size_t n, uint64_t time_ns) {
    auto const bytes = 3 * n * sizeof(float);
    return static_cast<double>(bytes) / time_ns;
}

void parse_args(int argc, char** argv, size_t& n, size_t& loop, size_t& chunk) {
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);

        if (arg.size() >= 7 && arg.substr(0, 7) == "--size=") {
            auto const val = std::stol(arg.substr(7));
            if (val == 0) {
                n = 1;
            }
            if (val <= 0) {
                n = -val;
            } else {
                n = val * 1024 * 1024;
            }
        } else if (arg.size() >= 7 && arg.substr(0, 7) == "--loop=") {
            loop = std::stoul(arg.substr(7));
        } else if (arg.size() >= 8 && arg.substr(0, 8) == "--chunk=") {
            chunk = std::stoul(arg.substr(8));
        } else {
            show_msg_fmt("Unknown option: %s\n", argv[i]);
            app_exit(1);
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    t_start = std::chrono::high_resolution_clock::now();
    show_msg("program starts");

    size_t n = 64 * 1024 * 1024;
    size_t loop = 10;
    size_t chunk = 0;
    parse_args(argc, argv, n, loop, chunk);
    show_msg_fmt("   n = %zu, loop = %zu, chunk = %zu\n", n, loop, chunk);

    show_msg(">> malloc on CPU");
    float* h_a = nullptr;
    posix_memalign((void**)&h_a, 2097152, n * sizeof(float));
    float* h_b = nullptr;
    posix_memalign((void**)&h_b, 2097152, n * sizeof(float));
    float* h_c = nullptr;
    posix_memalign((void**)&h_c, 2097152, n * sizeof(float));
    float* v_c = nullptr;
    posix_memalign((void**)&v_c, 2097152, n * sizeof(float));
    show_msg_fmt("   allocated size = %.2f GB\n",
                 (4 * n * sizeof(float)) / 1024.0 / 1024.0 / 1024.0);
    show_msg_fmt("   array size     = 3 x %.2f GB\n",
                 (n * sizeof(float)) / 1024.0 / 1024.0 / 1024.0);
    show_msg("<< malloc on CPU");

    show_msg(">> compute the initial data");
    {
        std::mt19937 g(1);
        initialize(h_a, h_b, n, g);
        zerofill(h_c, n);
        zerofill(v_c, n);
        show_msg_fmt("   a[0] = %f, a[N-1] = %f\n", h_a[0], h_a[n - 1]);
        show_msg_fmt("   a[0] = %f, b[N-1] = %f\n", h_b[0], h_b[n - 1]);
        show_msg_fmt("   c[0] = %f, c[N-1] = %f\n", h_c[0], h_c[n - 1]);
    }
    show_msg("<< compute the initial data");

    std::vector<uint64_t> ts(loop);

    show_msg(">> run benchmark");
    run_benchmark(h_a, h_b, h_c, n, loop, chunk, ts.data());
    show_msg("<< run benchmark");

    for (size_t i = 0; i < loop; i++) {
        show_msg_fmt("   run[%2zu] : %10.3f us, %6.2f GB/s\n", i, ts.at(i) / 1.0e3,
                     compute_bw(n, ts.at(i)));
    }

    std::stable_sort(ts.begin(), ts.end());
    show_msg_fmt("   best    : %10.3f us, %6.2f GB/s\n", ts.at(0) / 1.0e3,
                 compute_bw(n, ts.at(0)));

    show_msg(">> compute the initial data");
    {
        std::mt19937 g(1);
        initialize(h_a, h_b, n, g);
        zerofill(v_c, n);
        show_msg_fmt("   a[0] = %f, a[N-1] = %f\n", h_a[0], h_a[n - 1]);
        show_msg_fmt("   a[0] = %f, b[N-1] = %f\n", h_b[0], h_b[n - 1]);
        show_msg_fmt("   c[0] = %f, c[N-1] = %f\n", v_c[0], v_c[n - 1]);
    }
    show_msg("<< compute the initial data");

    show_msg(">> compute the verification data");
    vecadd_verify(h_a, h_b, v_c, n);
    show_msg_fmt("   c[0] = %f, c[N-1] = %f\n", v_c[0], v_c[n - 1]);
    show_msg("<< compute the verification data");

    show_msg(">> verification");
    show_msg_fmt("   c[0] = %f, c[N-1] = %f\n", h_c[0], h_c[n - 1]);
    auto const ok = run_verify(v_c, h_c, n);
    show_msg("<< verification");
    show_msg_fmt("verification : %s\n", ok ? "pass" : "FAIL");

    app_exit(ok ? 0 : 1);
}
