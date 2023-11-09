#include "common.hpp"

#define TEST_SZ(...)                                                             \
    {                                                                            \
        sz = -1;                                                                 \
        sycl::buffer<size_t, 1> x(&sz, {1});                                     \
        q.submit([&](sycl::handler& h) {                                         \
            sycl::accessor<size_t, 1, sycl::access_mode::write> xx(x, h);        \
            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) __VA_ARGS__); \
        });                                                                      \
    }

#define TEST_ELEM(...)                                                           \
    {                                                                            \
        elem = int(-1);                                                          \
        sycl::buffer<int, 1> x(&elem, {1});                                      \
        q.submit([&](sycl::handler& h) {                                         \
            sycl::accessor<int, 1, sycl::access_mode::write> xx(x, h);           \
            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) __VA_ARGS__); \
        });                                                                      \
    }

#define TEST_VEC(...)                                                            \
    {                                                                            \
        vec = vec_t(-1);                                                         \
        sycl::buffer<vec_t, 1> x(&vec, {1});                                     \
        q.submit([&](sycl::handler& h) {                                         \
            sycl::accessor<vec_t, 1, sycl::access_mode::write> xx(x, h);         \
            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) __VA_ARGS__); \
        });                                                                      \
    }

#define TEST_BOOLVEC(...)                                                        \
    {                                                                            \
        boolvec = boolvec_t(false);                                              \
        sycl::buffer<boolvec_t, 1> x(&boolvec, {1});                             \
        q.submit([&](sycl::handler& h) {                                         \
            sycl::accessor<boolvec_t, 1, sycl::access_mode::write> xx(x, h);     \
            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) __VA_ARGS__); \
        });                                                                      \
    }

TEST_CASE("vec", "") {
    using vec_t = sycl::vec<int, 4>;
    using halfvec_t = sycl::vec<int, 2>;
    using boolvec_t = sycl::vec<bool, 4>;

    sycl::queue q;
    size_t sz;
    vec_t vec;
    boolvec_t boolvec;
    int elem;

    TEST_SZ({
        vec_t v;
        xx[0] = v.byte_size();
    })

    REQUIRE(sz == sizeof(int) * 4);

    TEST_SZ({
        vec_t v;
        xx[0] = v.size();
    })

    REQUIRE(sz == 4);

    TEST_VEC({
        vec_t v(9999);
        xx[0] = v;
    })

    REQUIRE(vec[0] == 9999);
    REQUIRE(vec[1] == 9999);
    REQUIRE(vec[2] == 9999);
    REQUIRE(vec[3] == 9999);

    TEST_VEC({
        vec_t v(1, 2, 3, 4);
        xx[0] = v;
    })

    REQUIRE(vec[0] == 1);
    REQUIRE(vec[1] == 2);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 4);

    TEST_VEC({
        halfvec_t v1(2, 3);
        vec_t v(1, v1, 4);
        xx[0] = v;
    })

    REQUIRE(vec[0] == 1);
    REQUIRE(vec[1] == 2);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 4);

    TEST_VEC({
        halfvec_t v1(1, 2), v2(3, 4);
        vec_t v(v1, v2);
        xx[0] = v;
    })

    REQUIRE(vec[0] == 1);
    REQUIRE(vec[1] == 2);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 4);

    TEST_VEC({
        vec_t v1(1), v2(2);
        v1 += v2;
        xx[0] = v1;
    })

    REQUIRE(vec[0] == 3);
    REQUIRE(vec[1] == 3);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 3);

    TEST_VEC({
        vec_t v1(1), v2(2);
        xx[0] = v1 + v2;
    })

    REQUIRE(vec[0] == 3);
    REQUIRE(vec[1] == 3);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 3);

    TEST_BOOLVEC({
        vec_t v1(1), v2(1);
        xx[0] = v1 == v2;
    });

    REQUIRE(boolvec[0] == true);
    REQUIRE(boolvec[1] == true);
    REQUIRE(boolvec[2] == true);
    REQUIRE(boolvec[3] == true);

    TEST_BOOLVEC({
        vec_t v1(1), v2(2);
        xx[0] = v1 != v2;
    });

    REQUIRE(boolvec[0] == true);
    REQUIRE(boolvec[1] == true);
    REQUIRE(boolvec[2] == true);
    REQUIRE(boolvec[3] == true);

    TEST_ELEM({
        vec_t v(1, 2, 3, 4);
        xx[0] = v.x();
    })

    REQUIRE(elem == 1);

    TEST_ELEM({
        vec_t v(1, 2, 3, 4);
        xx[0] = v.y();
    })

    REQUIRE(elem == 2);

    TEST_ELEM({
        vec_t v(1, 2, 3, 4);
        xx[0] = v.z();
    })

    REQUIRE(elem == 3);

    TEST_ELEM({
        vec_t v(1, 2, 3, 4);
        xx[0] = v.w();
    })

    REQUIRE(elem == 4);

    TEST_ELEM({
        vec_t v(1, 2, 3, 4);
        xx[0] = v.a();
    })

    REQUIRE(elem == 4);
}
