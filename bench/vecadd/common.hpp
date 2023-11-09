#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>

extern std::chrono::high_resolution_clock::time_point t_start;

inline void show_timer() {
    auto const t_now = std::chrono::high_resolution_clock::now();
    auto const delta = t_now - t_start;
    auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
    fprintf(stderr, "[%10.3f] ", ns / 1.0e6);
}

inline void show_msg(char const* msg) {
    show_timer();
    fprintf(stderr, "%s\n", msg);
}

#define show_msg_fmt(format, ...)               \
    ({                                          \
        show_timer();                           \
        fprintf(stderr, (format), __VA_ARGS__); \
    })

inline void app_exit(int ret) {
    show_msg_fmt("program exits (%d)\n", ret);
    std::exit(ret);
}

inline void split_vecadd(size_t n, std::vector<size_t>& offsets, std::vector<size_t>& sizes,
                         int n_split) {
    offsets.resize(n_split);
    sizes.resize(n_split);

    for (int i = 0; i < n_split; i++) {
        if (i == 0) {
            offsets.at(i) = 0;
        } else {
            offsets.at(i) = std::accumulate(sizes.begin(), sizes.begin() + i, size_t(0),
                                            std::plus<size_t>());
        }

        // sizes.at(i) = ((n / n_split) + 255) / 256 * 256;
        if (i < n % n_split) {
            sizes.at(i) = n / n_split + (n % n_split ? 1 : 0);
        } else {
            sizes.at(i) = n / n_split;
        }
    }

    printf("offsets = [");
    for (int i = 0; i < n_split; i++) {
        printf("%zu, ", offsets.at(i));
    }
    printf("]\n");

    printf("sizes = [");
    for (int i = 0; i < n_split; i++) {
        printf("%zu, ", sizes.at(i));
    }
    printf("] = %zu\n",
           std::accumulate(sizes.begin(), sizes.end(), size_t(0), std::plus<size_t>()));
}
