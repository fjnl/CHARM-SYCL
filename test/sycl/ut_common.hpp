#pragma once

#include <random>
#include <vector>
#include <boost/ut.hpp>
#include <sycl/sycl.hpp>

template <>
auto boost::ut::cfg<boost::ut::override> =
    boost::ut::runner<boost::ut::reporter<boost::ut::printer>>{};

template <int Value>
struct int_ {
    static constexpr auto value = Value;

    constexpr operator int() const {
        return Value;
    }
};

inline constinit auto DIMS = std::tuple<int_<1>, int_<2>, int_<3>>{};

template <int D>
int pow(int x) {
    int r = 1;
    for (int d = 0; d < D; ++d) {
        r *= x;
    }
    return r;
}

template <int D>
sycl::range<D> make_range(size_t n) {
    if constexpr (D == 1) {
        return {n};
    } else if constexpr (D == 2) {
        return {n, n};
    } else {
        return {n, n, n};
    }
}

template <int D>
sycl::range<D> make_range(int r0, int r1, int r2) {
    if constexpr (D == 1) {
        return sycl::range(r0);
    } else if constexpr (D == 2) {
        return sycl::range(r0, r1);
    } else {
        return sycl::range(r0, r1, r2);
    }
}

template <int D>
sycl::id<D> make_id() {
    if constexpr (D == 1) {
        return {1};
    } else if constexpr (D == 2) {
        return {1, 2};
    } else {
        return {1, 2, 3};
    }
}

template <int D>
sycl::id<D> make_id(size_t i0) {
    if constexpr (D == 1) {
        return {i0};
    } else if constexpr (D == 2) {
        return {i0, i0};
    } else {
        return {i0, i0, i0};
    }
}

template <int D, class F>
void iterate(sycl::range<D> const& r, F const& f) {
    size_t seq = 0;

    if constexpr (D == 1) {
        for (size_t i = 0; i < r[0]; i++) {
            f(i, 0, 0, seq);
            seq++;
        }
    } else if constexpr (D == 2) {
        for (size_t i = 0; i < r[0]; i++)
            for (size_t j = 0; j < r[1]; j++) {
                f(i, j, 0, seq);
                seq++;
            }
    } else {
        for (size_t i = 0; i < r[0]; i++)
            for (size_t j = 0; j < r[1]; j++)
                for (size_t k = 0; k < r[2]; k++) {
                    f(i, j, k, seq);
                    seq++;
                }
    }
}

inline bool iris() {
    if (auto env_rts = getenv("CHARM_SYCL_RTS");
        env_rts && (strcmp(env_rts, "IRIS") == 0 || strcmp(env_rts, "IRIS-DMEM") == 0)) {
        return true;
    }
    return false;
}

struct skip_if_t {
    bool skip;

    template <class Test>
    inline friend auto operator/(skip_if_t sif, Test test) {
        using namespace boost::ut;

        if (sif.skip) {
            return boost::ut::skip / test;
        } else {
            return test;
        }
    }
};

inline skip_if_t skip_if(bool skip) {
    return skip_if_t{skip};
}

template <class T>
std::vector<T> random_values(size_t n, T min, T max) {
    std::random_device seed;
    std::mt19937 g(seed());
    std::uniform_int_distribution<T> d(min, max);
    std::vector<T> res;

    res.reserve(n);
    for (size_t i = 0; i < n; i++) {
        res.push_back(d(g));
    }
    return res;
}

namespace ut = boost::ut;
using namespace boost::ut;

namespace boost::ut {
template <int D>
inline ut::printer& operator<<(ut::printer& pr, sycl::range<D> const& range) {
    pr << "range(" << range[0];
    if constexpr (D > 0) {
        pr << ", " << range[1];
    }
    if constexpr (D > 1) {
        pr << ", " << range[2];
    }
    pr << ")";
    return pr;
}
}  // namespace boost::ut
