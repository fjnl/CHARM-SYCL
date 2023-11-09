#include "common.hpp"

TEST_CASE("buffer", "[sycl][buffer]") {
    auto const r = sycl::range<3>(100, 100, 100);

    sycl::buffer<int, 3> buff(r);

    REQUIRE(buff.get_range() == r);

    // sycl::host_accessor<int, 3, sycl::access_mode::read_write> x(buff);
}
