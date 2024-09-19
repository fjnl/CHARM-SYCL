#include "ut_common.hpp"

template <int D>
void testcase(sycl::queue& q) {
    int constexpr N = 3;
    int constexpr M = D >= 2 ? 4 : 1;
    int constexpr L = D == 3 ? 5 : 1;
    int constexpr K = 10;
    int constexpr KK = 100;
    int constexpr X = 1;

    std::vector<int> host(N * M * L);

    {
        sycl::buffer<int, D> buffer(host.data(), make_range<D>(N, M, L));
        sycl::queue q;

        q.submit([&](sycl::handler& h) {
            auto out = buffer.get_access(h);

            h.parallel_for(buffer.get_range(), make_id<D>(X), [=](sycl::id<D> const& id) {
                if constexpr (D == 1) {
                    out[id[0] - X] = (id[0] - X);
                } else if constexpr (D == 2) {
                    out[{id[0] - X, id[1] - X}] = (id[0] - X) * K + (id[1] - X);
                } else {
                    out[{id[0] - X, id[1] - X, id[2] - X}] =
                        (id[0] - X) * KK + (id[1] - X) * K + (id[2] - X);
                }
            });
        });
    }

    for (int i = 0, l = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for (int k = 0; k < L; k++, l++) {
                if constexpr (D == 1) {
                    expect(_i(host.at(l)) == i) << "i=" << i;
                } else if constexpr (D == 2) {
                    expect(_i(host.at(l)) == i * K + j) << "i=" << i << "j=" << j;
                } else {
                    expect(_i(host.at(l)) == i * KK + j * K + k)
                        << "i=" << i << "j=" << j << "k=" << k;
                }
            }
        }
    }
}

int main() {
    sycl::queue q;

    "parallel_for3 - 1"_test = [&]() {
        testcase<1>(q);
    };
    "parallel_for3 - 2"_test = [&]() {
        testcase<2>(q);
    };
    "parallel_for3 - 3"_test = [&]() {
        testcase<3>(q);
    };

    return 0;
}
