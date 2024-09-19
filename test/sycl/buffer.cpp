#include "ut_common.hpp"

int main() {
    "buffer"_test = []() {
        auto const r = sycl::range<3>(100, 100, 100);

        sycl::buffer<int, 3> buff(r);

        expect(buff.get_range() == r);
    };

    return 0;
}
