#pragma once
#include <utility>
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

// The vec<T, N> type must be transformable into devices by the compiler.
// It is required for the compiler to support all AST nodes inside the type.

namespace detail {

#define VEC_EXPAND(...) __VA_ARGS__

// TODO: If fhe compiler supports LambdaExpr, this macro would be better.
#define VEC_FOREACH_METHOD(name, fn_args_decl, fn_args, expr)                          \
private:                                                                               \
    template <class... Args>                                                           \
    void CHARM_SYCL_INLINE name(VEC_EXPAND fn_args_decl) {                             \
        name(std::make_integer_sequence<unsigned, NumElements>(), VEC_EXPAND fn_args); \
    }                                                                                  \
    template <unsigned I, unsigned... J, class... Args>                                \
    void CHARM_SYCL_INLINE name(std::integer_sequence<unsigned, I, J...>,              \
                                VEC_EXPAND fn_args_decl) {                             \
        name##_<I>(VEC_EXPAND fn_args);                                                \
        if constexpr (sizeof...(J) > 0) {                                              \
            name(std::integer_sequence<unsigned, J...>(), VEC_EXPAND fn_args);         \
        }                                                                              \
    }                                                                                  \
                                                                                       \
protected:                                                                             \
    template <unsigned I>                                                              \
    void CHARM_SYCL_INLINE name##_(VEC_EXPAND fn_args_decl) {                          \
        expr;                                                                          \
    }

#define VEC_COMPOUND_OP(method_name, op, rhs_type, expr)                \
    VEC_FOREACH_METHOD(method_name, (rhs_type const& rhs), (rhs), expr) \
public:                                                                 \
    inline friend vec_t& operator op(vec_t& lhs, rhs_type const& rhs) { \
        lhs.method_name(rhs);                                           \
        return lhs;                                                     \
    }

#define VEC_BINARY_OP(op)                                                 \
public:                                                                   \
    inline friend vec_t operator op(vec_t const& lhs, vec_t const& rhs) { \
        vec_t temp(lhs);                                                  \
        temp op## = rhs;                                                  \
        return temp;                                                      \
    }                                                                     \
    inline friend vec_t operator op(vec_t const& lhs, DataT const& rhs) { \
        vec_t temp(lhs);                                                  \
        temp op## = rhs;                                                  \
        return temp;                                                      \
    }

#define VEC_COMPARE_OP(method_name, op, rhs_type, expr)                                    \
    VEC_FOREACH_METHOD(method_name, (bool_t & res, rhs_type const& rhs), (res, rhs), expr) \
    inline friend bool_t operator op(vec_t& lhs, rhs_type const& rhs) {                    \
        bool_t res;                                                                        \
        lhs.method_name(res, rhs);                                                         \
        return res;                                                                        \
    }

// TODO: half
template <class _T>
auto constexpr is_vec_floating = std::is_same_v<_T, float> || std::is_same_v<_T, double>;

template <class DataT, int NumElements>
struct vec_base {
    constexpr vec_base() = default;

    explicit constexpr CHARM_SYCL_INLINE vec_base(DataT const& arg) {
        fill(arg);
    }

    template <class... Args>
    constexpr CHARM_SYCL_INLINE vec_base(Args const&... args) {
        assign_values<0u>(args...);
    }

    constexpr vec_base(vec_base const&) = default;

    constexpr vec_base(vec_base&&) = default;

    constexpr vec_base& operator=(vec_base const&) = default;

    constexpr vec_base& operator=(vec_base&&) = default;

protected:
    template <class _U, int _N>
    friend struct vec_base;

    VEC_FOREACH_METHOD(fill, (DataT const& arg), (arg), vec_[I] = arg)
    VEC_FOREACH_METHOD(copy, (vec_base const& arg), (arg), vec_[I] = arg.vec_[I])

    template <unsigned ElemIdx, unsigned VecIdx, class _T, int _NT, class... Args>
    inline void CHARM_SYCL_INLINE assign_vec(vec_base<_T, _NT> const& vecval,
                                             Args const&... args) {
        if constexpr (VecIdx == _NT) {
            if constexpr (sizeof...(Args) > 0) {
                assign_values<ElemIdx>(args...);
            }
        } else {
            static_assert(ElemIdx < NumElements, "initial values are too long");
            vec_[ElemIdx] = vecval.vec_[VecIdx];
            assign_vec<ElemIdx + 1, VecIdx + 1>(vecval, args...);
        }
    }

    template <unsigned ElemIdx, class _T, class... Args>
    inline void CHARM_SYCL_INLINE assign_values(_T const& val, Args const&... args) {
        if constexpr (std::is_convertible_v<_T, DataT>) {
            static_assert(ElemIdx < NumElements, "initial values are too long");
            vec_[ElemIdx] = val;
            if constexpr (sizeof...(Args) > 0) {
                assign_values<ElemIdx + 1>(args...);
            }
        } else {
            assign_vec<ElemIdx, 0u>(val, args...);
        }
    }

    DataT vec_[NumElements];
};

#define VEC_REF(name, idx)      \
    DataT& name() {             \
        return this->vec_[idx]; \
    }                           \
    DataT const& name() const { \
        return this->vec_[idx]; \
    }

template <template <class, int> class Derived, class DataT, int NumElements, class Base>
struct vec_x_accessor : Base {
    using Base::Base;

    VEC_REF(x, 0)
};

template <template <class, int> class Derived, class DataT, int NumElements, class Base>
struct vec_y_accessor : Base {
    using Base::Base;

    VEC_REF(y, 1)
};

template <template <class, int> class Derived, class DataT, int NumElements, class Base>
struct vec_z_accessor : Base {
    using Base::Base;

    VEC_REF(z, 2)
};

template <template <class, int> class Derived, class DataT, int NumElements, class Base>
struct vec_w_accessor : Base {
    using Base::Base;

    VEC_REF(w, 3)
};

template <template <class, int> class Derived, class DataT, int NumElements, class Base>
struct vec_rgba_accessor : Base {
    using Base::Base;

    VEC_REF(r, 0)
    VEC_REF(g, 1)
    VEC_REF(b, 2)
    VEC_REF(a, 3)
};

template <template <class, int> class Derived, class DataT, int NumElements, class Base>
struct vec_non_floating_ops : Base {
    using Base::Base;

private:
    using vec_t = Derived<DataT, NumElements>;

public:
    VEC_BINARY_OP(%);

    VEC_COMPOUND_OP(mod_eq_vec, %=, vec_t, this->vec_[I] %= rhs.vec_[I]);
    VEC_COMPOUND_OP(mod_eq_dat, %=, DataT, this->vec_[I] %= rhs);
};

template <template <class, int> class Derived, class DataT, int NumElements, class Base>
struct vec_common_ops : Base {
    using Base::Base;

private:
    using vec_t = Derived<DataT, NumElements>;
    using bool_t = Derived<bool, NumElements>;

public:
    VEC_BINARY_OP(+);
    VEC_BINARY_OP(-);
    VEC_BINARY_OP(*);
    VEC_BINARY_OP(/);

    VEC_COMPOUND_OP(plus_eq_vec, +=, vec_t, this->vec_[I] += rhs.vec_[I]);
    VEC_COMPOUND_OP(plus_eq_dat, +=, DataT, this->vec_[I] += rhs);
    VEC_COMPOUND_OP(minus_eq_vec, -=, vec_t, this->vec_[I] -= rhs.vec_[I]);
    VEC_COMPOUND_OP(minus_eq_dat, -=, DataT, this->vec_[I] -= rhs);
    VEC_COMPOUND_OP(mul_eq_vec, *=, vec_t, this->vec_[I] *= rhs.vec_[I]);
    VEC_COMPOUND_OP(mul_eq_dat, *=, DataT, this->vec_[I] *= rhs);
    VEC_COMPOUND_OP(div_eq_vec, /=, vec_t, this->vec_[I] /= rhs.vec_[I]);
    VEC_COMPOUND_OP(div_eq_dat, /=, DataT, this->vec_[I] /= rhs);

    VEC_COMPARE_OP(cmp_eq_vec, ==, vec_t, res[I] = this->vec_[I] == rhs.vec_[I]);
    VEC_COMPARE_OP(cmp_eq_dat, ==, DataT, res[I] = this->vec_[I] == rhs);
    VEC_COMPARE_OP(cmp_ne_vec, !=, vec_t, res[I] = this->vec_[I] != rhs.vec_[I]);
    VEC_COMPARE_OP(cmp_ne_dat, !=, DataT, res[I] = this->vec_[I] != rhs);
    VEC_COMPARE_OP(cmp_lt_vec, <, vec_t, res[I] = this->vec_[I] < rhs.vec_[I]);
    VEC_COMPARE_OP(cmp_lt_dat, <, DataT, res[I] = this->vec_[I] < rhs);
    VEC_COMPARE_OP(cmp_le_vec, <=, vec_t, res[I] = this->vec_[I] <= rhs.vec_[I]);
    VEC_COMPARE_OP(cmp_le_dat, <=, DataT, res[I] = this->vec_[I] <= rhs);
    VEC_COMPARE_OP(cmp_gt_vec, >, vec_t, res[I] = this->vec_[I] > rhs.vec_[I]);
    VEC_COMPARE_OP(cmp_gt_dat, >, DataT, res[I] = this->vec_[I] > rhs);
    VEC_COMPARE_OP(cmp_ge_vec, >=, vec_t, res[I] = this->vec_[I] >= rhs.vec_[I]);
    VEC_COMPARE_OP(cmp_ge_dat, >=, DataT, res[I] = this->vec_[I] >= rhs);
};

template <bool Cond, template <template <class, int> class, class, int, class> class T,
          template <class, int> class Derived, class DataT, int NumElements, class Base>
struct vec_cond : std::conditional_t<Cond, T<Derived, DataT, NumElements, Base>, Base> {
private:
    using base_t = std::conditional_t<Cond, T<Derived, DataT, NumElements, Base>, Base>;

public:
    using base_t::base_t;
};

// clang-format off
template <template <class, int> class Derived, class DataT, int NumElements>
using vec_impl_t =
    detail::vec_common_ops<Derived, DataT, NumElements,
    vec_cond<!is_vec_floating<DataT>, detail::vec_non_floating_ops, Derived, DataT, NumElements,
    vec_cond<NumElements == 4, detail::vec_rgba_accessor, Derived, DataT, NumElements,
    vec_cond<4 <= NumElements && NumElements <= 4, detail::vec_w_accessor, Derived, DataT, NumElements,
    vec_cond<3 <= NumElements && NumElements <= 4, detail::vec_z_accessor, Derived, DataT, NumElements,
    vec_cond<2 <= NumElements && NumElements <= 4, detail::vec_y_accessor, Derived, DataT, NumElements,
    vec_cond<1 <= NumElements && NumElements <= 4, detail::vec_x_accessor, Derived, DataT, NumElements,
    detail::vec_base<DataT, NumElements
    >>>>>>>>;
// clang-format on

}  // namespace detail

template <class DataT, int NumElements>
struct vec : detail::vec_impl_t<vec, DataT, NumElements> {
private:
    using impl_type = detail::vec_impl_t<vec, DataT, NumElements>;

public:
    using impl_type::impl_type;

    using element_type = DataT;

    static inline constexpr size_t CHARM_SYCL_INLINE byte_size() noexcept {
        return sizeof(DataT) * NumElements;
    }

    static inline constexpr size_t CHARM_SYCL_INLINE size() noexcept {
        return NumElements;
    }

    inline size_t CHARM_SYCL_INLINE get_size() const {
        return byte_size();
    }

    inline size_t CHARM_SYCL_INLINE get_count() const {
        return size();
    }

    inline DataT& CHARM_SYCL_INLINE operator[](int index) {
        return this->vec_[index];
    }

    inline const DataT& CHARM_SYCL_INLINE operator[](int index) const {
        return this->vec_[index];
    }
};

#define VEC_TYPE_ALIASES(type)    \
    using type##2 = vec<type, 2>; \
    using type##3 = vec<type, 3>; \
    using type##4 = vec<type, 4>; \
    using type##8 = vec<type, 8>; \
    using type##16 = vec<type, 16>;

VEC_TYPE_ALIASES(int)
VEC_TYPE_ALIASES(float)
VEC_TYPE_ALIASES(double)

#undef VEC_TYPE_ALIASES
#undef VEC_REF
#undef VEC_COMPARE_OP
#undef VEC_BINARY_OP
#undef VEC_COMPOUND_OP
#undef VEC_FOREACH_METHOD
#undef VEC_EXPAND

CHARM_SYCL_END_NAMESPACE
