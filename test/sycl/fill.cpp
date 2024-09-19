#include "ut_common.hpp"

template <int D>
void run_tests(sycl::queue& q) {
    skip_if(iris()) / "fill"_test = [&]() {
        std::vector<int> host(pow<D>(10), -1);
        int const fill_val = 12345;

        {
            sycl::buffer buffer(host.data(), make_range<D>(10));
            q.submit([&](sycl::handler& h) {
                h.fill(buffer.get_access(h), fill_val);
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            expect(eq(host.at(seq), fill_val))
                << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
        });
    };

    skip_if(iris()) / "fill partial"_test = [&]() {
        std::vector<int> host(pow<D>(10), -1);
        int const fill_val = 12345;

        {
            sycl::buffer buffer(host.data(), make_range<D>(10));
            q.submit([&](sycl::handler& h) {
                h.fill(buffer.get_access(h, make_range<D>(5), make_id<D>()), fill_val);
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            if ((1 <= i && i < 6) && (D < 2 || (2 <= j && j < 7)) &&
                (D < 3 || (3 <= k && k < 8))) {
                expect(eq(host.at(seq), fill_val))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
            } else {
                expect(eq(host.at(seq), -1))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
            }
        });
    };
}

int main() {
    sycl::queue q;

    run_tests<1>(q);
    run_tests<2>(q);
    run_tests<3>(q);

    return 0;
}
