#include "ut_common.hpp"

int main() {
    sycl::queue q;

    "tag"_test = [&](auto tag) {
        int result = -1;
        sycl::buffer<int, 1> x(&result, {1});

        q.submit([&](sycl::handler& h) {
            sycl::accessor xx(x, h, tag);
        });
        q.wait();

        expect(true);
    } | std::tuple{sycl::read_only, sycl::write_only, sycl::read_write};

    "host tag"_test = [&](auto tag) {
        int result = -1;
        sycl::buffer<int, 1> x(&result, {1});

        x.get_host_access(tag);

        expect(true);
    } | std::tuple{sycl::read_only, sycl::write_only, sycl::read_write};

    return 0;
}
