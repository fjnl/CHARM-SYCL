#include "ut_common.hpp"

int main() {
    "platform"_test = []() {
        expect(sycl::platform::get_platforms().size() > 0_i);

        sycl::platform p;

        expect(neq(p.get_info<sycl::info::platform::name>(), std::string("")));

        expect(neq(p.get_info<sycl::info::platform::vendor>(), std::string("")));

        expect(neq(p.get_info<sycl::info::platform::version>(), std::string("")));
    };

    return 0;
}
