#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

template <int _N, class _T, int... _Indexes>
struct swizzled_vec {
    friend struct vec<_T, _N>;

    operator _T() const
        requires(sizeof...(_Indexes) == 1)
    {
        return detail::__charm_sycl_vec_ix<_T, _N>(v_->v_, (_Indexes + ...));
    }

    swizzled_vec& operator=(_T v)
        requires(sizeof...(_Indexes) == 1)
    {
        detail::__charm_sycl_vec_aS<_T, _N>(
            &v_->v_, (_Indexes + ...),
            detail::__charm_sycl_vec_ix<_T, _N>(v->v_, (_Indexes + ...)));
        return *this;
    }

    swizzled_vec& operator+=(_T v)
        requires(sizeof...(_Indexes) == 1)
    {
        detail::__charm_sycl_vec_aS<_T, _N>(
            &v_->v_, (_Indexes + ...),
            detail::__charm_sycl_vec_ix<_T, _N>(v_->v_, (_Indexes + ...)) + v);
        return *this;
    }

private:
    explicit swizzled_vec(vec<_T, _N>* v) : v_(v) {}

    vec<_T, _N>* v_;
};

template <class T, int N>
struct vec_proxy {
private:
    using value_type = std::remove_const_t<T>;

    using ptr_t =
        std::conditional_t<std::is_const_v<T>, vec<value_type, N> const*, vec<value_type, N>*>;

public:
    vec_proxy(ptr_t vec, int i) : vec_(vec), i_(i) {}

    operator value_type() const {
        return detail::__charm_sycl_vec_ix<value_type, N>(vec_->v_, i_);
    }

    vec_proxy operator=(T val) const
        requires(!std::is_const_v<T>)
    {
        detail::__charm_sycl_vec_aS<T, N>(&vec_->v_, i_, val);
        return *this;
    }

private:
    ptr_t vec_;
    int i_;
};

template <int PackIdx, int ElemIdx, int Offset, class... Args>
constexpr auto vec_init_arg() {
    using T = std::tuple_element_t<PackIdx, std::tuple<Args...>>;

    if constexpr (is_vec_v<T>) {
        if constexpr (Offset <= ElemIdx && ElemIdx < Offset + T::size()) {
            return std::make_tuple(PackIdx, true, ElemIdx - Offset);
        } else {
            return vec_init_arg<PackIdx + 1, ElemIdx, Offset + T::size(), Args...>();
        }
    } else {
        if constexpr (Offset == ElemIdx) {
            return std::make_tuple(PackIdx, false, -1);
        } else {
            return vec_init_arg<PackIdx + 1, ElemIdx, Offset + 1, Args...>();
        }
    }
}

template <class DataT, int I, bool IsVec, int VecIdx>
struct vec_init_impl {
    template <class T, class... Ts>
    constexpr auto operator()(T, Ts const&... args) const {
        return vec_init_impl<DataT, I - 1, IsVec, VecIdx>()(args...);
    }
};

template <class DataT>
struct vec_init_impl<DataT, 0, false, -1> {
    template <class T, class... Ts>
    constexpr auto operator()(T const& val, Ts const&...) const {
        return static_cast<DataT>(val);
    }
};

template <class DataT, int VecIdx>
struct vec_init_impl<DataT, 0, true, VecIdx> {
    template <class T, int N, class... Ts>
    constexpr auto operator()(vec<T, N> const& val, Ts const&...) const {
        static_assert(VecIdx >= 0);
        return static_cast<DataT>(val[VecIdx]);
    }
};

template <class DataT, size_t I, class... Args>
struct vec_init_ {
    static constexpr auto info = vec_init_arg<0, I, 0, Args...>();
    static constexpr int arg_idx = std::get<0>(info);
    static constexpr bool is_vec = std::get<1>(info);
    static constexpr int vec_idx = std::get<2>(info);

    constexpr auto operator()(Args const&... args) const {
        return vec_init_impl<DataT, arg_idx, is_vec, vec_idx>()(args...);
    }
};

template <class Vec, class DataT, size_t... Is, class... Args>
auto vec_init(std::index_sequence<Is...>, Args const&... args) {
    return (Vec){vec_init_<DataT, Is, Args...>()(args...)...};
}

}  // namespace detail

template <class DataT, int NumElements>
template <class... Args>
constexpr vec<DataT, NumElements>::vec(Args const&... args)
    : v_(detail::vec_init<vec_t, DataT>(std::make_index_sequence<NumElements>(), args...)) {}

template <class DataT, int NumElements>
template <int... _Indexes>
detail::swizzled_vec<NumElements, DataT, _Indexes...> vec<DataT, NumElements>::swizzle() const {
    return detail::swizzled_vec<NumElements, DataT, _Indexes...>(
        const_cast<vec<DataT, NumElements>*>(this));
}

template <class DataT, int NumElements>
detail::vec_proxy<DataT, NumElements> vec<DataT, NumElements>::operator[](int index) {
    return {this, index};
}

template <class DataT, int NumElements>
detail::vec_proxy<DataT const, NumElements> vec<DataT, NumElements>::operator[](
    int index) const {
    return {this, index};
}

CHARM_SYCL_END_NAMESPACE
