#include "ut_common.hpp"

template <class T, T Value>
struct kernel {
    explicit kernel(sycl::accessor<T, 1, sycl::access_mode::read_write> const& x) : x_(x) {}

    void operator()(sycl::id<1> const&) const {
        x_[0] += Value;
    }

    sycl::accessor<T, 1, sycl::access_mode::read_write> x_;
};

int main() {
    sycl::queue q;

    "functor3"_test = [&]() {
        int result = -1;
        {
            sycl::buffer<int, 1> x(&result, {1});

            q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::read_write> xx(x, h);

                h.parallel_for(sycl::range(1), kernel<int, 10>(xx));
            });

            q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::read_write> xx(x, h);

                h.parallel_for(sycl::range(1), kernel<int, 20>(xx));
            });
        }

        expect(eq(result, -1 + 10 + 20));
    };

    return 0;
}
