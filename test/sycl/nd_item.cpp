#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "nd_item"_test = [q]() mutable {
        std::vector<int> data(192, -1);

        {
            sycl::buffer<int, 1> x(data.data(), {data.size()});

            auto ev = q.submit([&](sycl::handler& h) {
                auto res = x.get_access(h);

                sycl::nd_range<3> ndr(sycl::range(4, 6, 8), sycl::range(2, 3, 4));

                h.parallel_for(ndr, [=](sycl::nd_item<3> const& item) {
                    res[item.get_global_linear_id()] = item.get_global_linear_id();
                });
            });
        }

        for (size_t i = 0; i < data.size(); i++) {
            expect(_i(data.at(i)) == i) << "i=" << i;
        }
    };
}
