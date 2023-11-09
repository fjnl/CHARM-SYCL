#include "common.hpp"

TEST_CASE("queue_profiling", "") {
    sycl::queue q({sycl::property::queue::enable_profiling()});

    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.single_task([=] {
                xx[0] = 1;
            });
        });

        auto const t0 = ev.get_profiling_info<sycl::info::event_profiling::command_submit>();
        auto const t1 = ev.get_profiling_info<sycl::info::event_profiling::command_start>();
        auto const t2 = ev.get_profiling_info<sycl::info::event_profiling::command_end>();

        CHECKED_IF(t0 == -1) {
            // some backends don't support profiling yet
            REQUIRE(t0 == -1);
            REQUIRE(t1 == -1);
            REQUIRE(t2 == -1);
        }
        CHECKED_ELSE(t0 == -1) {
            REQUIRE(t0 != 0);
            REQUIRE(t1 != 0);
            REQUIRE(t2 != 0);
            REQUIRE(t1 >= t0);
            REQUIRE(t2 >= t1);
        }
    }

    REQUIRE(result == 1);
}
