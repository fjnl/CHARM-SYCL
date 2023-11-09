#include "common.hpp"

TEST_CASE("platform", "[sycl][platform]") {
    REQUIRE(sycl::platform::get_platforms().size() > 0);

    sycl::platform p;

    SECTION("name") {
        REQUIRE(p.get_info<sycl::info::platform::name>() != "");
    }

    SECTION("vendor") {
        REQUIRE(p.get_info<sycl::info::platform::vendor>() != "");
    }

    SECTION("version") {
        REQUIRE(p.get_info<sycl::info::platform::version>() != "");
    }
}
