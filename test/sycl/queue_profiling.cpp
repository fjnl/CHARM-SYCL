#include "ut_common.hpp"

int main() {
    sycl::queue q({sycl::property::queue::enable_profiling()});

    "queue_profiling"_test = [&]() {
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.single_task([=] {
                    xx[0] = 1;
                });
            });

            auto const t0 =
                ev.get_profiling_info<sycl::info::event_profiling::command_submit>();
            auto const t1 = ev.get_profiling_info<sycl::info::event_profiling::command_start>();
            auto const t2 = ev.get_profiling_info<sycl::info::event_profiling::command_end>();

            if (t0 == -1) {
                // some backends don't support profiling yet
                expect(t0 == -1_i);
                expect(t1 == -1_i);
                expect(t2 == -1_i);
            } else {
                expect(t0 != 0_i);
                expect(t1 != 0_i);
                expect(t2 != 0_i);
                expect(t1 >= _i(t0));
                expect(t2 >= _i(t1));
            }
        }

        expect(result == 1_i);
    };

    return 0;
}
