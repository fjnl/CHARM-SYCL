#include "common.hpp"

TEMPLATE_TEST_CASE("tag", "", decltype(sycl::read_only), decltype(sycl::write_only),
                   decltype(sycl::read_write)) {
    sycl::queue q;
    int result = -1;
    sycl::buffer<int, 1> x(&result, {1});

    q.submit([&](sycl::handler& h) {
        sycl::accessor xx(x, h, TestType{});
    });
    q.wait();

    SUCCEED();
}

TEMPLATE_TEST_CASE("host tag", "", decltype(sycl::read_only), decltype(sycl::write_only),
                   decltype(sycl::read_write)) {
    sycl::queue q;
    int result = -1;
    sycl::buffer<int, 1> x(&result, {1});

    x.get_host_access(TestType{});

    SUCCEED();
}
