#include "ut_common.hpp"

int one() {
    return 1;
}

template <class... Args>
int va(Args... xs) {
    return [... xs = xs]() {
        return (xs + ...);
    }();
}

int main() {
    sycl::queue q;

    "lambda"_test = [&]() {
        int result = -1;

        {
            sycl::buffer<int, 1> x(&result, {1});

            auto ev = q.submit([&](sycl::handler& h) {
                sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

                h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                    int x = 0;

                    x += []() {
                        return 1;
                    }();

                    int y = 1;

                    x += [y]() {
                        return y;
                    }();

                    x += [y]() {
                        return [y]() {
                            return y;
                        }();
                    }();

                    x += [y]() {
                        return [z = y]() {
                            return z;
                        }();
                    }();

                    x += [z = y]() {
                        return z;
                    }();

                    x += [z = one()]() {
                        return z;
                    }();

                    x += [](int z) {
                        return z;
                    }(1);

                    x += [](auto z) {
                        return z;
                    }(1);

                    x += [](auto&&... z) {
                        return (z + ...);
                    }(1, 2, 3);

                    auto const f = [y](int z) {
                        return y + z;
                    };
                    x += f(1);

                    x += va(1, 2, 3);

                    xx[0] = x;
                });
            });
        }

        expect(result == 22_i);
    };

    return 0;
}
