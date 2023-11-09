#include "common.hpp"

TEST_CASE("nd_item", "") {
    sycl::queue q;
    std::vector<int> data(16, -1);

    {
        sycl::buffer<int, 1> x(data.data(), {data.size()});

        auto ev = q.submit([&](sycl::handler& h) {
            auto res = x.get_access(h);

            sycl::nd_range<2> ndr(sycl::range(4, 4), sycl::range(2, 2));

            h.parallel_for(ndr, [=](sycl::nd_item<2> const& item) {
                res[item.get_global_linear_id()] = item.get_global_linear_id();
            });
        });
    }

    for (int i = 0; i < 16; i++) {
        REQUIRE(data.at(i) == i);
    }
}
