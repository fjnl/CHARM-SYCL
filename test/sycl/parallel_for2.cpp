#include "common.hpp"

TEMPLATE_TEST_CASE_SIG("parallel_for2", "", ((int D), D), 1, 2, 3) {
    int constexpr N = 3;
    int constexpr M = D >= 2 ? 4 : 1;
    int constexpr L = D == 3 ? 5 : 1;
    int constexpr K = 10;
    int constexpr KK = 100;

    std::vector<int> host(N * M * L);

    {
        sycl::buffer<int, D> buffer(host.data(), make_range<D>(N, M, L));
        sycl::queue q;

        q.submit([&](sycl::handler& h) {
            auto out = buffer.get_access(h);

            h.parallel_for(buffer.get_range(), [=](sycl::id<D> const& id) {
                if constexpr (D == 1) {
                    out[id] = id[0];
                } else if constexpr (D == 2) {
                    out[id] = id[0] * K + id[1];
                } else {
                    out[id] = id[0] * KK + id[1] * K + id[2];
                }
            });
        });
    }

    for (int i = 0, l = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for (int k = 0; k < L; k++, l++) {
                CAPTURE(i, j, k, l, N, M, L);

                if constexpr (D == 1) {
                    REQUIRE(host.at(l) == i);
                } else if constexpr (D == 2) {
                    REQUIRE(host.at(l) == i * K + j);
                } else {
                    REQUIRE(host.at(l) == i * KK + j * K + k);
                }
            }
        }
    }
}
