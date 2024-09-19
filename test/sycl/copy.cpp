#include "ut_common.hpp"

template <int D>
void run_tests(sycl::queue& q) {
    skip_if(iris()) / "copy"_test = [&]() {
        std::vector<int> h_src(pow<D>(10), -1);
        std::vector<int> h_dst(pow<D>(10), -1);

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            h_src.at(seq) = i * 100 + j * 10 + k;
        });

        {
            sycl::buffer src(h_src.data(), make_range<D>(10));
            sycl::buffer dst(h_dst.data(), make_range<D>(10));

            q.submit([&](sycl::handler& h) {
                h.copy(src.get_access(h), dst.get_access(h));
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            expect(eq(h_dst.at(seq), i * 100 + j * 10 + k))
                << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
        });
    };

    skip_if(iris()) / "copy_partial"_test = [&]() {
        std::vector<int> h_src(pow<D>(10), -1);
        std::vector<int> h_dst(pow<D>(10), -1);

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            h_src.at(seq) = i * 100 + j * 10 + k;
        });

        {
            sycl::buffer src(h_src.data(), make_range<D>(10));
            sycl::buffer dst(h_dst.data(), make_range<D>(10));

            q.submit([&](sycl::handler& h) {
                h.copy(src.get_access(h, make_range<D>(5), make_id<D>()),
                       dst.get_access(h, make_range<D>(5), make_id<D>()));
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            if ((1 <= i && i < 6) && (D < 2 || (2 <= j && j < 7)) &&
                (D < 3 || (3 <= k && k < 8))) {
                expect(eq(h_dst.at(seq), i * 100 + j * 10 + k))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
            } else {
                expect(eq(h_dst.at(seq), -1))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
            }
        });
    };

    skip_if(iris()) / "copy_partial_2"_test = [&]() {
        std::vector<int> h_src(pow<D>(10), -1);
        std::vector<int> h_dst(pow<D>(10), -1);

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            h_src.at(seq) = i * 100 + j * 10 + k;
        });

        {
            sycl::buffer src(h_src.data(), make_range<D>(10));
            sycl::buffer dst(h_dst.data(), make_range<D>(10));

            q.submit([&](sycl::handler& h) {
                h.copy(src.get_access(h, make_range<D>(5)),
                       dst.get_access(h, make_range<D>(5), make_id<D>()));
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            if ((1 <= i && i < 6) && (D < 2 || (2 <= j && j < 7)) &&
                (D < 3 || (3 <= k && k < 8))) {
                expect(eq(h_dst.at(seq),
                          (i - 1) * 100 + (D < 2 ? 0 : j - 2) * 10 + (D < 3 ? 0 : k - 3)))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
            } else {
                expect(eq(h_dst.at(seq), -1))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
            }
        });
    };

    skip_if(iris()) / "copy_partial_3"_test = [&]() {
        std::vector<int> h_src(pow<D>(10), -1);
        std::vector<int> h_dst(pow<D>(10), -1);

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            h_src.at(seq) = i * 100 + j * 10 + k;
        });

        {
            sycl::buffer src(h_src.data(), make_range<D>(10));
            sycl::buffer dst(h_dst.data(), make_range<D>(10));

            q.submit([&](sycl::handler& h) {
                h.copy(src.get_access(h, make_range<D>(5), make_id<D>()),
                       dst.get_access(h, make_range<D>(5)));
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            if (i < 5 && j < 5 && k < 5) {
                expect(eq(h_dst.at(seq),
                          (i + 1) * 100 + (D < 2 ? 0 : j + 2) * 10 + (D < 3 ? 0 : k + 3)))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
            } else {
                expect(eq(h_dst.at(seq), -1))
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
