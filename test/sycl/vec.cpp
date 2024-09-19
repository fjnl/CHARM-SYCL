#include "ut_common.hpp"

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
        boolvec = boolvec_t(0);                                                  \
        sycl::buffer<boolvec_t, 1> x(&boolvec, {1});                             \
        q.submit([&](sycl::handler& h) {                                         \
            sycl::accessor<boolvec_t, 1, sycl::access_mode::write> xx(x, h);     \
            h.parallel_for(sycl::range(1), [=](sycl::id<1> const&) __VA_ARGS__); \
        });                                                                      \
    }

int main() {
    "vec -- size and align (Host)"_test = []() {
#define SIZE_TEST(T, N) expect(eq(sizeof(sycl::vec<T, N>), sizeof(T) * (N == 3 ? 4 : N)))
#define ALIGN_TEST(T, N) expect(eq(alignof(sycl::vec<T, N>), sizeof(sycl::vec<T, N>)))

#define TEST(T, N)   \
    SIZE_TEST(T, N); \
    ALIGN_TEST(T, N)

#define TEST_T(T) \
    TEST(T, 1);   \
    TEST(T, 2);   \
    TEST(T, 3);   \
    TEST(T, 4);   \
    TEST(T, 8);   \
    TEST(T, 16)
        TEST_T(char);
        TEST_T(unsigned char);
        TEST_T(short);
        TEST_T(unsigned short);
        TEST_T(int);
        TEST_T(unsigned int);
        TEST_T(long);
        TEST_T(unsigned long);
        TEST_T(long long);
        TEST_T(unsigned long long);
        TEST_T(float);
        TEST_T(double);
    };

    using vec_t = sycl::vec<int, 4>;
    using halfvec_t = sycl::vec<int, 2>;
    using boolvec_t = sycl::vec<int, 4>;
    sycl::queue q;

    "vec"_test = [&]() {
        size_t sz;
        vec_t vec;
        boolvec_t boolvec;
        int elem;

        TEST_SZ({
            vec_t v;
            xx[0] = v.byte_size();
        })

        expect(eq(sz, sizeof(int) * 4));

        TEST_SZ({
            vec_t v;
            xx[0] = v.size();
        })

        expect(sz == 4_i);

        TEST_VEC({
            vec_t v(9999);
            xx[0] = v;
        })

        expect(vec[0] == 9999_i);
        expect(vec[1] == 9999_i);
        expect(vec[2] == 9999_i);
        expect(vec[3] == 9999_i);

        TEST_VEC({
            vec_t v(1, 2, 3, 4);
            xx[0] = v;
        })

        expect(vec[0] == 1_i);
        expect(vec[1] == 2_i);
        expect(vec[2] == 3_i);
        expect(vec[3] == 4_i);

        TEST_VEC({
            halfvec_t v1(2, 3);
            vec_t v(1, v1, 4);
            xx[0] = v;
        })

        expect(vec[0] == 1_i);
        expect(vec[1] == 2_i);
        expect(vec[2] == 3_i);
        expect(vec[3] == 4_i);

        TEST_VEC({
            halfvec_t v1(1, 2), v2(3, 4);
            vec_t v(v1, v2);
            xx[0] = v;
        })

        expect(vec[0] == 1_i);
        expect(vec[1] == 2_i);
        expect(vec[2] == 3_i);
        expect(vec[3] == 4_i);

        TEST_VEC({
            vec_t v1(1), v2(2);
            v1 += v2;
            xx[0] = v1;
        })

        expect(vec[0] == 3_i);
        expect(vec[1] == 3_i);
        expect(vec[2] == 3_i);
        expect(vec[3] == 3_i);

        TEST_VEC({
            vec_t v1(1), v2(2);
            xx[0] = v1 + v2;
        })

        expect(vec[0] == 3_i);
        expect(vec[1] == 3_i);
        expect(vec[2] == 3_i);
        expect(vec[3] == 3_i);

        TEST_BOOLVEC({
            vec_t v1(1), v2(1);
            xx[0] = v1 == v2;
        });

        expect(boolvec[0] == -1_i);
        expect(boolvec[1] == -1_i);
        expect(boolvec[2] == -1_i);
        expect(boolvec[3] == -1_i);

        TEST_BOOLVEC({
            vec_t v1(1), v2(2);
            xx[0] = v1 != v2;
        });

        expect(boolvec[0] == -1_i);
        expect(boolvec[1] == -1_i);
        expect(boolvec[2] == -1_i);
        expect(boolvec[3] == -1_i);

        TEST_ELEM({
            vec_t v(1, 2, 3, 4);
            xx[0] = v.x();
        })

        expect(elem == 1_i);

        TEST_ELEM({
            vec_t v(1, 2, 3, 4);
            xx[0] = v.y();
        })

        expect(elem == 2_i);

        TEST_ELEM({
            vec_t v(1, 2, 3, 4);
            xx[0] = v.z();
        })

        expect(elem == 3_i);

        TEST_ELEM({
            vec_t v(1, 2, 3, 4);
            xx[0] = v.w();
        })

        expect(elem == 4_i);

        TEST_ELEM({
            vec_t v(1, 2, 3, 4);
            xx[0] = v.a();
        })

        expect(elem == 4_i);
    };

    return 0;
}
