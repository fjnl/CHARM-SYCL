#include "common.hpp"

TEST_CASE("local_accessor", "") {
    if (auto env_rts = getenv("CHARM_SYCL_RTS"); env_rts && strcmp(env_rts, "IRIS") == 0) {
        SKIP();
    }

    sycl::queue q;

    int result = -1;
    static int constexpr S = 16;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::local_accessor<int, 1> smem({S}, h);
            auto out = x.get_access(h);

            h.parallel_for(sycl::nd_range<1>({S}, {S}), [=](sycl::nd_item<1> const& item) {
                smem[item.get_local_linear_id()] = item.get_local_linear_id();

                item.barrier(sycl::access::fence_space::local_space);

                if (item.get_local_linear_id() == 0) {
                    int sum = 0;
                    for (int i = 0; i < S; i++) {
                        sum += smem[i];
                    }
                    out[0] = sum;
                }
            });
        });

        ev.wait();
    }

    int expected = 0;
    for (int i = 0; i < S; i++) {
        expected += i;
    }
    REQUIRE(result == expected);
}
