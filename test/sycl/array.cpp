#include "common.hpp"

TEST_CASE("array", "") {
    sycl::queue q;
    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int arr[10];
                for (int i = 0; i < 10; i++) {
                    arr[i] = i;
                }
                xx[0] = arr[9];
            });
        });
    }

    REQUIRE(result == 9);
}

TEST_CASE("array2", "") {
    sycl::queue q;
    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int arr[5][5];
                for (int i = 0; i < 5; i++) {
                    for (int j = 0; j < 5; j++) {
                        arr[i][j] = i * 5 + j;
                    }
                }
                xx[0] = arr[4][4];
            });
        });
    }

    REQUIRE(result == 24);
}

static void f(int x[5], int base) {
    for (int i = 0; i < 5; i++) {
        x[i] = base + i;
    }
}

TEST_CASE("array3", "") {
    sycl::queue q;
    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int arr[5][5];
                for (int i = 0; i < 5; i++) {
                    f(arr[i], i * 5);
                }
                xx[0] = arr[4][4];
            });
        });
    }

    REQUIRE(result == 24);
}

static void g(int x[][5]) {
    for (int i = 0; i < 5; i++) {
        int* x_ptr = x[i];
        for (int j = 0; j < 5; j++) {
            x_ptr[j] = i * 5 + j;
        }
    }
}

TEST_CASE("array4", "") {
    sycl::queue q;
    int result = -1;

    {
        sycl::buffer<int, 1> x(&result, {1});

        auto ev = q.submit([&](sycl::handler& h) {
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);

            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) {
                int arr[5][5];
                g(arr);
                xx[0] = arr[4][4];
            });
        });
    }

    REQUIRE(result == 24);
}
