#include "common.hpp"

template <int D>
sycl::range<D> single_range() {
    if constexpr (D == 1) {
        return {1};
    } else if constexpr (D == 2) {
        return {1, 1};
    } else {
        return {1, 1, 1};
    }
}

TEMPLATE_TEST_CASE_SIG("item", "", ((int D), D), (2)) {
    // TEMPLATE_TEST_CASE_SIG("item", "", ((int D), D), (1), (2), (3)) {
    sycl::queue q;

    int result = -1;

    {
        sycl::buffer<int, D> buffer(&result, single_range<D>());

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, D, sycl::access_mode::write> a(buffer, h);

            h.parallel_for(buffer.get_range(), [=](sycl::item<D> i) {
                a[i] = 12345;
            });
        });

        ev.wait();
    }

    REQUIRE(result == 12345);
}
