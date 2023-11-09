#include <type_traits>
#include "common.hpp"

template <class T>
inline T xabs(T x) {
    if constexpr (std::is_unsigned_v<T>) {
        return x;
    } else {
        return std::abs(x);
    }
}

template <class T, int D>
static T vecadd(sycl::range<D> range) {
    sycl::queue q;

    std::vector<T> a_host(range.size());
    std::vector<T> b_host(range.size());
    std::vector<T> c_host(range.size());

    for (size_t i = 0; i < range.size(); i++) {
        a_host.at(i) = i;
        b_host.at(i) = i;
        c_host.at(i) = 0;
    }

    {
        sycl::buffer<T, D> a_dev(a_host.data(), range);
        sycl::buffer<T, D> b_dev(b_host.data(), range);
        sycl::buffer<T, D> c_dev(c_host.data(), range);

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<T, D, sycl::access_mode::read> a(a_dev, h);
            sycl::accessor<T, D, sycl::access_mode::read> b(b_dev, h);
            sycl::accessor<T, D, sycl::access_mode::write> c(c_dev, h);

            h.parallel_for(range, [=](sycl::id<D> const& i) {
                c[i] = a[i] + b[i];
            });
        });

        ev.wait();
    }

    T err = 0;

    for (size_t i = 0; i < range.size(); i++) {
        err = std::max<T>(err, xabs(c_host.at(i) - (a_host.at(i) + b_host.at(i))));
    }
    return err;
}

TEMPLATE_TEST_CASE("vecadd", "", float) {
    REQUIRE(vecadd<TestType, 1>(sycl::range(1000)) == 0);
    REQUIRE(vecadd<TestType, 2>(sycl::range(10, 10)) == 0);
    REQUIRE(vecadd<TestType, 3>(sycl::range(10, 10, 10)) == 0);
}
