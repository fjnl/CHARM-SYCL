#include "ut_common.hpp"

template <int D>
void run_tests(sycl::queue& q) {
    skip_if(iris()) / "copy2_1"_test = [&]() {
        std::vector<int> h_src(pow<D>(10), -1);
        std::vector<int> h_dst(pow<D>(10), -1);

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            h_src.at(seq) = i * 100 + j * 10 + k;
        });

        {
            sycl::buffer src(h_src.data(), make_range<D>(10));

            q.submit([&](sycl::handler& h) {
                h.copy(src.get_access(h), h_dst.data());
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            expect(eq(h_dst.at(seq), i * 100 + j * 10 + k))
                << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
        });
    };

    skip_if(iris()) / "copy2_2"_test = [&]() {
        std::vector<int> h_src(pow<D>(10), -1);
        std::vector<int> h_dst(pow<D>(10), -1);

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            h_src.at(seq) = i * 100 + j * 10 + k;
        });

        {
            sycl::buffer dst(h_dst.data(), make_range<D>(10));

            q.submit([&](sycl::handler& h) {
                h.copy(h_src.data(), dst.get_access(h));
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            expect(eq(h_dst.at(seq), i * 100 + j * 10 + k))
                << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
        });
    };

    skip_if(iris()) / "copy2_3"_test = [&]() {
        std::vector<int> h_src(pow<D>(10), -1);
        std::vector<int> h_dst(pow<D>(10), -1);

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            h_src.at(seq) = i * 100 + j * 10 + k;
        });

        {
            sycl::buffer src(h_src.data(), make_range<D>(10));

            q.submit([&](sycl::handler& h) {
                h.copy(src.get_access(h, make_range<D>(5), make_id<D>()), h_dst.data());
            });
        }

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            size_t ii, jj = 0, kk = 0;

            if constexpr (D == 1) {
                ii = seq;
            } else if constexpr (D == 2) {
                ii = seq / 5;
                jj = seq % 5;
            } else {
                ii = seq / 25;
                jj = seq / 5 % 5;
                kk = seq % 5;
            }

            if (seq < pow<D>(5)) {
                expect(eq(h_dst.at(seq), (ii + 1) * 100 + (jj + (D < 2 ? 0 : 2)) * 10 +
                                             (kk + (D < 3 ? 0 : 3))))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
                ;
            } else {
                expect(eq(h_dst.at(seq), -1))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
            }
        });
    };

    skip_if(iris()) / "copy2_4"_test = [&]() {
        std::vector<int> h_src(pow<D>(10), -1);
        std::vector<int> h_dst(pow<D>(10), -1);

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            h_src.at(seq) = i * 100 + j * 10 + k;
        });

        {
            sycl::buffer dst(h_dst.data(), make_range<D>(10));

            q.submit([&](sycl::handler& h) {
                h.copy(h_src.data(), dst.get_access(h, make_range<D>(5), make_id<D>()));
            });
        }

        size_t src_idx = 0;

        iterate(make_range<D>(10), [&](auto i, auto j, auto k, auto seq) {
            if ((1 <= i && i < 6) && (D < 2 || (2 <= j && j < 7)) &&
                (D < 3 || (3 <= k && k < 8))) {
                expect(eq(h_dst.at(seq), h_src.at(src_idx)))
                    << "i =" << i << ", j =" << j << ", k =" << k << ", seq =" << seq;
                src_idx++;
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
