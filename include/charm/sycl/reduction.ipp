#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

template <class DataT, class OpT, class BinaryOperation, int Dimensions>
struct reducer;

template <class DataT, class OpT, class PlusT>
struct reducer<DataT, OpT, plus<PlusT>, 0> {
    explicit reducer(accessor<DataT, 1, access_mode::discard_write> const& acc) : acc_(acc) {}

    explicit reducer(accessor<DataT, 1, access_mode::discard_write> const& acc,
                     plus<PlusT> const&)
        : acc_(acc) {}

    reducer(reducer const&) = delete;
    reducer(reducer&&) = delete;
    reducer& operator=(reducer const&) = delete;
    reducer& operator=(reducer const&&) = delete;

    inline void initialize() {
        if constexpr (std::is_same_v<OpT, float>) {
            runtime::__charm_sycl_reduce_initialize_f(&val_);
        } else if constexpr (std::is_same_v<OpT, double>) {
            runtime::__charm_sycl_reduce_initialize_d(&val_);
        }
    }

    inline reducer& combine(OpT const& partial) {
        if constexpr (std::is_same_v<OpT, float>) {
            runtime::__charm_sycl_reduce_combine_f(&val_, partial);
        } else if constexpr (std::is_same_v<OpT, double>) {
            runtime::__charm_sycl_reduce_combine_d(&val_, partial);
        }
        return *this;
    }

    inline void finalize(bool is_leader) {
        if constexpr (std::is_same_v<OpT, float>) {
            runtime::__charm_sycl_reduce_finalize_f(acc_.get_pointer(), val_, is_leader);
        } else if constexpr (std::is_same_v<OpT, double>) {
            runtime::__charm_sycl_reduce_finalize_d(acc_.get_pointer(), val_, is_leader);
        }
    }

    static inline constexpr auto is_zero_identity() {
        if constexpr (std::is_same_v<OpT, float>) {
            return true;
        } else if constexpr (std::is_same_v<OpT, double>) {
            return true;
        }
    }

    static inline constexpr auto identity() {}

    static inline bool check_zero(OpT const& v) {
        if constexpr (std::is_same_v<OpT, float>) {
            return std::fpclassify(v) == FP_ZERO;
        } else if constexpr (std::is_same_v<OpT, double>) {
            return std::fpclassify(v) == FP_ZERO;
        }
    }

    // constexpr std::enable_if_t<has_known_identity_v<T>, T> identity() const;

    friend reducer& operator+=(reducer& self, OpT const& rhs) {
        return self.combine(rhs);
    }

private:
    friend struct sycl::handler;

    reducer clone() const {
        return reducer(acc_);
    }

    accessor<DataT, 1, access_mode::discard_write> acc_;
    DataT val_;
} CHARM_SYCL_DEVICE_COPYABLE;

// TODO:
// operator []
// TODO:
// operator *=
// operator &=
// operator |=
// operator ^=
// operator ++

template <class DataT, int Dimensions, class AllocatorT, class BinaryOperation>
inline auto reduction(buffer<DataT, Dimensions, AllocatorT> vars, handler& cgh,
                      std::optional<DataT>, BinaryOperation&& combiner, property_list const&) {
    using OpT = std::remove_reference_t<
        std::remove_cv_t<std::invoke_result_t<BinaryOperation, DataT, DataT>>>;
    using R = reducer<DataT, OpT, BinaryOperation, 0>;

    auto fill_ev =
        runtime::impl_access::make<sycl::queue>(runtime::impl_access::get_queue(cgh))
            .submit([&](handler& filler) {
                auto acc = vars.get_access(filler, write_only);
#ifndef __SYCL_DEVICE_ONLY__
                auto acc_ = runtime::impl_access::get_impl(acc);
                runtime::impl_access::get_impl(filler)->fill_zero(acc_, sizeof(DataT));
#endif
            });

    cgh.depends_on(fill_ev);

    auto acc = vars.get_access(cgh, write_only);

    return R(acc, std::forward<BinaryOperation>(combiner));
}

}  // namespace detail

template <class BufferT, class BinaryOperation>
inline auto reduction(BufferT vars, handler& cgh, BinaryOperation&& combiner,
                      property_list const& prop_list = {}) {
    return detail::reduction(vars, cgh, {}, std::forward<BinaryOperation>(combiner), prop_list);
}

template <class BufferT, class BinaryOperation>
auto reduction(BufferT vars, handler& cgh, typename BufferT::value_type const& identity,
               BinaryOperation&& combiner, property_list const& prop_list = {}) {
    return detail::reduction(vars, cgh, std::make_optional(identity),
                             std::forward<BinaryOperation>(combiner), prop_list);
}

CHARM_SYCL_END_NAMESPACE
