#include "ut_common.hpp"

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

int main() {
    sycl::queue q;

    "item"_test = [&]() {
        int result = -1;
        {
            sycl::buffer<int, 2> buffer(&result, single_range<2>());

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 2, sycl::access_mode::write> a(buffer, h);

                h.parallel_for(buffer.get_range(), [=](sycl::item<2> i) {
                    a[i] = 12345;
                });
            });

            ev.wait();
        }

        expect(result == 12345_i);
    };

    return 0;
}
