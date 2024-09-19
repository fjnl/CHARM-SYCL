#pragma once
#include <charm/sycl.hpp>
#include <charm/sycl/vec_fwd.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

template <int N, class T, int... Indexes>
struct swizzled_vec;

template <class T, int N>
struct vec_proxy;

template <class>
struct is_vec : std::false_type {};

template <class T, int N>
struct is_vec<vec<T, N>> : std::true_type {};

template <class T>
inline constexpr bool is_vec_v = is_vec<T>::value;

template <class T>
concept Vec = is_vec_v<T>;

}  // namespace detail

template <class DataT, int NumElements>
struct alignas(detail::vec_size<DataT, NumElements>) vec {
    static_assert(NumElements == 1 || NumElements == 2 || NumElements == 3 ||
                  NumElements == 4 || NumElements == 8 || NumElements == 16);

private:
    template <int N, class T, int... Indexes>
    friend struct detail::swizzled_vec;

    template <class T, int N>
    friend struct detail::vec_proxy;

    template <class T, int N>
    friend struct vec;

    using elem_t = std::conditional_t<std::is_same_v<DataT, bool>, char, DataT>;

    using bool_vec_t =
        vec<std::conditional_t<
                sizeof(DataT) == 1, int8_t,
                std::conditional_t<sizeof(DataT) == 2, int16_t,
                                   std::conditional_t<sizeof(DataT) == 4, int32_t, uint64_t>>>,
            NumElements>;

    typedef elem_t __attribute__((ext_vector_type(NumElements))) vec_t;
    // typedef elem_t __attribute__((vector_size(sizeof(DataT) * NumElements))) vec_t;

public:
    using element_type = DataT;
    using vector_t = vec_t;

    vec() = default;

    explicit constexpr vec(DataT arg)
        : v_(detail::__charm_sycl_vec_splat<DataT, NumElements>(arg)) {}

    template <class... Args>
    constexpr vec(Args const&... args);

    constexpr vec(vec const&) = default;

    explicit vec(vector_t v) : v_(v) {}

    // template <int _N, class _U, size_t... _Indexes>
    // vec(detail::swizzled_vec<_N, _U, _Indexes...> sw)
    //     requires(std::convertible_to<_U, DataT> && sizeof...(_Indexes) == NumElements);

    operator vector_t() const {
        return v_;
    }

    static constexpr size_t byte_size() noexcept {
        if constexpr (NumElements == 3) {
            return 4 * sizeof(DataT);
        } else {
            return NumElements * sizeof(DataT);
        }
    }

    static constexpr size_t size() noexcept {
        return NumElements;
    }

    size_t get_size() const {
        return byte_size();
    }

    size_t get_count() const {
        return size();
    }

    // convert()

    // as()

    template <int... Indexes>
    detail::swizzled_vec<NumElements, DataT, Indexes...> swizzle() const;

    detail::vec_proxy<DataT, NumElements> operator[](int index);

    detail::vec_proxy<DataT const, NumElements> operator[](int index) const;

    vec& operator=(vec const& rhs) {
        v_ = rhs.v_;
        return *this;
    }

    vec& operator=(DataT const& rhs) {
        v_ = rhs;
        return *this;
    }

    // template <int _N, class _U, int... _Indexes>
    // vec& operator=(detail::swizzled_vec<_N, _U, _Indexes...> const&)
    //     requires(std::convertible_to<_U, DataT> && sizeof...(_Indexes) == NumElements);

#include <charm/sycl/vec_ops.hpp>
#include <charm/sycl/vec_swizzle.hpp>

private:
    mutable vec_t v_;
} CHARM_SYCL_DEVICE_COPYABLE;

CHARM_SYCL_END_NAMESPACE
