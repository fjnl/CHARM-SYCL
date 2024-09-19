#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <class T = void>
struct plus {
    template <class Lhs, class Rhs>
    auto operator()(Lhs&& lhs, Rhs&& rhs) const
        -> std::conditional_t<std::is_void_v<T>, decltype(lhs + rhs), T>;
};

namespace runtime {
bool __charm_sycl_is_reduce_leader_1();
bool __charm_sycl_is_reduce_leader_2();
bool __charm_sycl_is_reduce_leader_3();

void __charm_sycl_reduce_initialize_f(float*);
void __charm_sycl_reduce_combine_f(float*, float);
void __charm_sycl_reduce_finalize_f(float*, float, bool);

void __charm_sycl_reduce_initialize_d(double*);
void __charm_sycl_reduce_combine_d(double*, double);
void __charm_sycl_reduce_finalize_d(double*, double, bool);
}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
