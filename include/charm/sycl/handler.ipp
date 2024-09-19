#pragma once

#include <charm/sycl.hpp>
#include <charm/sycl/fnv1a.hpp>

#if defined(__SYCL_DEVICE_ONLY__) && !__has_builtin(__builtin_sycl_unique_stable_name)
#    error __builtin_sycl_unique_stable_name() is not supported by the compiler.
#endif

CHARM_SYCL_BEGIN_NAMESPACE

inline handler::handler(queue const& q)
    : q_(runtime::impl_access::get_impl(q)), impl_(runtime::make_handler(q_)) {}

void handler::depends_on(event ev) {
    impl_->depends_on(runtime::impl_access::get_impl(ev));
}

template <class KernelName, class KernelType>
inline void handler::single_task(KernelType const& kernel) {
    using Name = std::conditional_t<std::is_same_v<KernelName, detail::unnamed_kernel>,
                                    std::remove_cvref_t<KernelType>, KernelName>;

    single_task_<Name>(kernel);
}

template <class KernelName, int Dimensions, class KernelType>
inline void handler::parallel_for_(range<Dimensions> const& range, KernelType const& kernel) {
#ifdef __SYCL_DEVICE_ONLY__
    static_assert(std::is_trivially_copyable_v<KernelType>,
                  "The kernel function must be a trivially copyable.");
#endif

    using Name = std::remove_cvref_t<KernelName>;
    size_t name_len;
    auto const* name = runtime::__charm_sycl_kernel_name<Name>(name_len);

    auto const h = detail::fnv1a(name, name_len + 3);
    impl_->parallel_for(detail::extend(range), name, h);

    do_bind<Name>(kernel);
}

template <class KernelName, int Dimensions, class KernelType>
inline void handler::parallel_for_(nd_range<Dimensions> const& range,
                                   KernelType const& kernel) {
#ifdef __SYCL_DEVICE_ONLY__
    static_assert(std::is_trivially_copyable_v<KernelType>,
                  "The kernel function must be a trivially copyable.");
#endif

    using Name = std::remove_cvref_t<KernelName>;
    size_t name_len;
    auto const* name = runtime::__charm_sycl_kernel_name<Name>(name_len);

    auto const h = detail::fnv1a(name, name_len + 3);
    impl_->parallel_for(detail::extend(range), name, h);

    do_bind<Name>(kernel);
}

template <class KernelName, class KernelType>
inline void handler::single_task_(KernelType const& kernel) {
#ifdef __SYCL_DEVICE_ONLY__
    static_assert(std::is_trivially_copyable_v<KernelType>,
                  "The kernel function must be a trivially copyable.");
#endif

    using Name = std::remove_cvref_t<KernelName>;
    size_t name_len;
    auto const* name = runtime::__charm_sycl_kernel_name<Name>(name_len);

    auto const h = detail::fnv1a(name, name_len + 3);

    impl_->single_task(name, h);

    do_bind<Name>(kernel);
}

namespace detail {
template <class F, class Head, class... Tail>
void reverse(F f, Head&& head, Tail&&... tail) {
    if constexpr (sizeof...(Tail) == 0) {
        f(std::forward<Head>(head));
    } else {
        reverse(
            [&](auto&&... rtail) {
                f(std::forward<decltype(rtail)>(rtail)..., std::forward<Head>(head));
            },
            std::forward<Tail>(tail)...);
    }
}
}  // namespace detail

template <class KernelName, int Dimensions, class... Rest>
void handler::parallel_for(range<Dimensions> const& range, Rest&&... rest) {
    using Args = std::tuple<std::remove_reference_t<std::remove_cv_t<Rest>>...>;

    if constexpr (std::tuple_size_v<Args> == 1) {
        //* parallel_for(range, kernel)
        parallel_for_1<KernelName>(range, rest...);
    } else {
        using Second = std::tuple_element_t<0, Args>;
        if constexpr (std::is_same_v<Second, id<1>> || std::is_same_v<Second, id<2>> ||
                      std::is_same_v<Second, id<3>>) {
            //* parallel_for(range, offset, kernel)
            parallel_for_2<KernelName>(range, rest...);
        } else {
            //* parallel_for(range, reducer..., kernel)
            detail::reverse(
                [&](auto&& fn, auto&&... reversed) {
                    detail::reverse(
                        [&](auto&&... reducers) {
                            parallel_for_reduce<KernelName>(
                                range, fn, std::make_index_sequence<sizeof...(reducers)>(),
                                reducers...);
                        },
                        std::forward<decltype(reversed)>(reversed)...);
                },
                std::forward<Rest>(rest)...);
        }
    }
}

template <class KernelName, int Dimensions, class KernelType>
void handler::parallel_for_1(range<Dimensions> const& range, KernelType& fn) {
#ifdef __SYCL_DEVICE_ONLY__
    static_assert(std::is_trivially_copyable_v<KernelType>,
                  "The kernel function must be a trivially copyable.");
#endif
    static_assert(Dimensions == 1 || Dimensions == 2 || Dimensions == 3);
    using Name = std::conditional_t<std::is_same_v<KernelName, detail::unnamed_kernel>,
                                    std::remove_cvref_t<KernelType>, KernelName>;

    if constexpr (Dimensions == 1) {
        parallel_for_<Name>(range, [fn, range0 = range[0]]() {
            sycl::range<1> r(range0);

            for (size_t i [[clang::annotate("charm_sycl_parallel_for 3")]] =
                     runtime::__charm_sycl_parallel_iter3_begin();
                 runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                 i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                runtime::__charm_sycl_kernel(&fn);

                fn(detail::make_item(r, sycl::id<1>(i)));
            }
        });
    } else if constexpr (Dimensions == 2) {
        parallel_for_<Name>(range, [fn, range1 = range[1], range0 = range[0]]() {
            sycl::range<2> r(range0, range1);

            for (size_t j [[clang::annotate("charm_sycl_parallel_for 2")]] =
                     runtime::__charm_sycl_parallel_iter2_begin();
                 runtime::__charm_sycl_parallel_iter2_cond(j, range1);
                 j = runtime::__charm_sycl_parallel_iter2_step(j)) {
                for (size_t i [[clang::annotate("charm_sycl_parallel_for 3")]] =
                         runtime::__charm_sycl_parallel_iter3_begin();
                     runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                     i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                    runtime::__charm_sycl_kernel(&fn);

                    fn(detail::make_item(r, sycl::id<2>(i, j)));
                }
            }
        });
    } else {
        parallel_for_<Name>(
            range, [fn, range2 = range[2], range1 = range[1], range0 = range[0]]() {
                sycl::range<3> r(range0, range1, range2);

                for (size_t k [[clang::annotate("charm_sycl_parallel_for 1")]] =
                         runtime::__charm_sycl_parallel_iter1_begin();
                     runtime::__charm_sycl_parallel_iter1_cond(k, range2);
                     k = runtime::__charm_sycl_parallel_iter1_step(k)) {
                    for (size_t j [[clang::annotate("charm_sycl_parallel_for 2")]] =
                             runtime::__charm_sycl_parallel_iter2_begin();
                         runtime::__charm_sycl_parallel_iter2_cond(j, range1);
                         j = runtime::__charm_sycl_parallel_iter2_step(j)) {
                        for (size_t i [[clang::annotate("charm_sycl_parallel_for 3")]] =
                                 runtime::__charm_sycl_parallel_iter3_begin();
                             runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                             i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                            runtime::__charm_sycl_kernel(&fn);

                            fn(detail::make_item(r, sycl::id<3>(i, j, k)));
                        }
                    }
                }
            });
    }
}

template <class KernelName, int Dimensions, class KernelType>
void handler::parallel_for_2(range<Dimensions> const& range, id<Dimensions> const& offset,
                             KernelType& fn) {
#ifdef __SYCL_DEVICE_ONLY__
    static_assert(std::is_trivially_copyable_v<KernelType>,
                  "The kernel function must be a trivially copyable.");
#endif
    static_assert(Dimensions == 1 || Dimensions == 2 || Dimensions == 3);
    using Name = std::conditional_t<std::is_same_v<KernelName, detail::unnamed_kernel>,
                                    std::remove_cvref_t<KernelType>, KernelName>;

    if constexpr (Dimensions == 1) {
        parallel_for_<Name>(range, [fn, offset0 = offset[0], range0 = range[0]]() {
            sycl::range<1> r(range0);

            for (size_t i [[clang::annotate("charm_sycl_parallel_for 3")]] =
                     runtime::__charm_sycl_parallel_iter3_begin();
                 runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                 i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                runtime::__charm_sycl_kernel(&fn);

                fn(detail::make_item(r, sycl::id<1>(offset0 + i)));
            }
        });
    } else if constexpr (Dimensions == 2) {
        parallel_for_<Name>(range, [fn, offset0 = offset[0], offset1 = offset[1],
                                    range1 = range[1], range0 = range[0]]() {
            sycl::range<2> r(range0, range1);

            for (size_t j [[clang::annotate("charm_sycl_parallel_for 2")]] =
                     runtime::__charm_sycl_parallel_iter2_begin();
                 runtime::__charm_sycl_parallel_iter2_cond(j, range1);
                 j = runtime::__charm_sycl_parallel_iter2_step(j)) {
                for (size_t i [[clang::annotate("charm_sycl_parallel_for 3")]] =
                         runtime::__charm_sycl_parallel_iter3_begin();
                     runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                     i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                    runtime::__charm_sycl_kernel(&fn);

                    fn(detail::make_item(r, sycl::id<2>(offset0 + i, offset1 + j)));
                }
            }
        });
    } else {
        parallel_for_<Name>(
            range, [fn, offset0 = offset[0], offset1 = offset[1], offset2 = offset[2],
                    range2 = range[2], range1 = range[1], range0 = range[0]]() {
                sycl::range<3> r(range0, range1, range2);

                for (size_t k [[clang::annotate("charm_sycl_parallel_for 1")]] =
                         runtime::__charm_sycl_parallel_iter1_begin();
                     runtime::__charm_sycl_parallel_iter1_cond(k, range2);
                     k = runtime::__charm_sycl_parallel_iter1_step(k)) {
                    for (size_t j [[clang::annotate("charm_sycl_parallel_for 2")]] =
                             runtime::__charm_sycl_parallel_iter2_begin();
                         runtime::__charm_sycl_parallel_iter2_cond(j, range1);
                         j = runtime::__charm_sycl_parallel_iter2_step(j)) {
                        for (size_t i [[clang::annotate("charm_sycl_parallel_for 3")]] =
                                 runtime::__charm_sycl_parallel_iter3_begin();
                             runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                             i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                            runtime::__charm_sycl_kernel(&fn);

                            fn(detail::make_item(
                                r, sycl::id<3>(offset0 + i, offset1 + j, offset2 + k)));
                        }
                    }
                }
            });
    }
}

template <class KernelName, int Dimensions, class KernelType, size_t... I, class... Reducers>
void handler::parallel_for_reduce(range<Dimensions> const& range, KernelType& fn,
                                  std::index_sequence<I...>, Reducers&... reducers) {
#ifdef __SYCL_DEVICE_ONLY__
    static_assert(std::is_trivially_copyable_v<KernelType>,
                  "The kernel function must be a trivially copyable.");
#endif
    static_assert(Dimensions == 1 || Dimensions == 2 || Dimensions == 3);
    using Name = std::conditional_t<std::is_same_v<KernelName, detail::unnamed_kernel>,
                                    std::remove_cvref_t<KernelType>, KernelName>;

    if constexpr (Dimensions == 1) {
        parallel_for_<Name>(
            range, [fn, range0 = range[0], ... reducers = reducers.clone()]() mutable {
                (reducers.initialize(), ...);

                sycl::range<1> r(range0);

                for (size_t i [[clang::annotate("charm_sycl_parallel_for top 3")]] =
                         runtime::__charm_sycl_parallel_iter3_begin();
                     runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                     i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                    runtime::__charm_sycl_kernel(&fn);

                    fn(detail::make_item(r, sycl::id<1>(i)), reducers...);
                }

                bool is_leader = runtime::__charm_sycl_is_reduce_leader_1();
                (reducers.finalize(is_leader), ...);
            });
    } else if constexpr (Dimensions == 2) {
        parallel_for_<Name>(range, [fn, range1 = range[1], range0 = range[0],
                                    ... reducers = reducers.clone()]() mutable {
            (reducers.initialize(), ...);

            sycl::range<2> r(range0, range1);

            for (size_t j [[clang::annotate("charm_sycl_parallel_for top 2")]] =
                     runtime::__charm_sycl_parallel_iter2_begin();
                 runtime::__charm_sycl_parallel_iter2_cond(j, range1);
                 j = runtime::__charm_sycl_parallel_iter2_step(j)) {
                for (size_t i [[clang::annotate("charm_sycl_parallel_for 3")]] =
                         runtime::__charm_sycl_parallel_iter3_begin();
                     runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                     i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                    runtime::__charm_sycl_kernel(&fn);

                    fn(detail::make_item(r, sycl::id<2>(i, j)), reducers...);
                }
            }

            bool is_leader = runtime::__charm_sycl_is_reduce_leader_2();
            (reducers.finalize(is_leader), ...);
        });
    } else {
        parallel_for_<Name>(range, [fn, range2 = range[2], range1 = range[1], range0 = range[0],
                                    ... reducers = reducers.clone()]() mutable {
            (reducers.initialize(), ...);

            sycl::range<3> r(range0, range1, range2);

            for (size_t k [[clang::annotate("charm_sycl_parallel_for top 1")]] =
                     runtime::__charm_sycl_parallel_iter1_begin();
                 runtime::__charm_sycl_parallel_iter1_cond(k, range1);
                 k = runtime::__charm_sycl_parallel_iter1_step(k)) {
                for (size_t j [[clang::annotate("charm_sycl_parallel_for 2")]] =
                         runtime::__charm_sycl_parallel_iter2_begin();
                     runtime::__charm_sycl_parallel_iter2_cond(j, range1);
                     j = runtime::__charm_sycl_parallel_iter2_step(j)) {
                    for (size_t i [[clang::annotate("charm_sycl_parallel_for 3")]] =
                             runtime::__charm_sycl_parallel_iter3_begin();
                         runtime::__charm_sycl_parallel_iter3_cond(i, range0);
                         i = runtime::__charm_sycl_parallel_iter3_step(i)) {
                        runtime::__charm_sycl_kernel(&fn);

                        fn(detail::make_item(r, sycl::id<3>(i, j, k)), reducers...);
                    }
                }
            }

            bool is_leader = runtime::__charm_sycl_is_reduce_leader_2();
            (reducers.finalize(is_leader), ...);
        });
    }
}

template <class KernelName, class WorkgroupFunctionType, int Dimensions>
void handler::parallel_for_work_group(range<Dimensions> const& numWorkGroups,
                                      range<Dimensions> const& workGroupSize,
                                      WorkgroupFunctionType const& fn) {
#ifdef __SYCL_DEVICE_ONLY__
    static_assert(std::is_trivially_copyable_v<WorkgroupFunctionType>,
                  "The kernel function must be a trivially copyable.");
#endif
    using Name = std::conditional_t<std::is_same_v<KernelName, detail::unnamed_kernel>,
                                    std::remove_cvref_t<WorkgroupFunctionType>, KernelName>;
    sycl::nd_range<Dimensions> ndr(numWorkGroups * workGroupSize, workGroupSize);

    if constexpr (Dimensions == 1) {
        parallel_for_<Name>(ndr, [fn]() {
            /* scope: wg entry */
            sycl::range<1> group_range(runtime::__charm_sycl_group_range3());
            sycl::range<1> local_range(0);
            sycl::id<1> group_id(runtime::__charm_sycl_group_id3());
            sycl::id<1> local_id(0);

            sycl::group<1> g(group_range, local_range, group_id, local_id);

            runtime::__charm_sycl_kernel(&fn);
            fn(g);
        });
    } else if constexpr (Dimensions == 2) {
        parallel_for_<Name>(ndr, [fn]() {
            /* scope: wg entry */
            sycl::range<2> group_range(runtime::__charm_sycl_group_range3(),
                                       runtime::__charm_sycl_group_range2());
            sycl::range<2> local_range(0, 0);
            sycl::id<2> group_id(runtime::__charm_sycl_group_id3(),
                                 runtime::__charm_sycl_group_id2());
            sycl::id<2> local_id(0, 0);

            sycl::group<2> g(group_range, local_range, group_id, local_id);

            runtime::__charm_sycl_kernel(&fn);
            fn(g);
        });
    } else {
        parallel_for_<Name>(ndr, [fn]() {
            /* scope: wg entry */
            sycl::range<3> group_range(runtime::__charm_sycl_group_range3(),
                                       runtime::__charm_sycl_group_range2(),
                                       runtime::__charm_sycl_group_range1());
            sycl::range<3> local_range(0, 0, 0);
            sycl::id<3> group_id(runtime::__charm_sycl_group_id3(),
                                 runtime::__charm_sycl_group_id2(),
                                 runtime::__charm_sycl_group_id1());
            sycl::id<3> local_id(0, 0, 0);

            sycl::group<3> g(group_range, local_range, group_id, local_id);

            runtime::__charm_sycl_kernel(&fn);
            fn(g);
        });
    }
}

template <class KernelName, int Dimensions, class KernelType>
void handler::parallel_for(nd_range<Dimensions> const& range, KernelType const& fn) {
    parallel_for_3<KernelName>(range, fn);
}

template <class KernelName, int Dimensions, class KernelType>
void handler::parallel_for_3(nd_range<Dimensions> const& range, KernelType const& fn) {
#ifdef __SYCL_DEVICE_ONLY__
    static_assert(std::is_trivially_copyable_v<KernelType>,
                  "The kernel function must be a trivially copyable.");
#endif
    using Name = std::conditional_t<std::is_same_v<KernelName, detail::unnamed_kernel>,
                                    std::remove_cvref_t<KernelType>, KernelName>;

    parallel_for_work_group<Name>(
        range.get_group_range(), range.get_local_range(), [fn](sycl::group<Dimensions> g) {
            /* scope: wg */
            g.parallel_for_work_item([fn](sycl::h_item<Dimensions> h_item) {
                /* scope: wi */
                fn(h_item.into_nd_item());
            });
        });
}

template <class SrcT, int SrcDim, access_mode SrcMode, target SrcTgt, class DestT>
void handler::copy(accessor<SrcT, SrcDim, SrcMode, SrcTgt> src, DestT* dest) {
    static_assert(std::is_same_v<SrcT, DestT>, "Type mismatch");
    static_assert(detail::is_readable(SrcMode), "Source is not writable");
    static_assert(SrcTgt == target::device, "Non-device copy is not supported");

#ifdef __SYCL_DEVICE_ONLY__
    (void)src;
    (void)dest;
#else
    impl_->copy(runtime::impl_access::get_impl(src), dest);
#endif
}

template <class SrcT, class DestT, int DestDim, access_mode DestMode, target DestTgt>
void handler::copy(const SrcT* src, accessor<DestT, DestDim, DestMode, DestTgt> dest) {
    static_assert(std::is_same_v<SrcT, DestT>, "Type mismatch");
    static_assert(detail::is_writable(DestMode), "Destination is not writable");
    static_assert(DestTgt == target::device, "Non-device copy is not supported");

#ifdef __SYCL_DEVICE_ONLY__
    (void)src;
    (void)dest;
#else
    impl_->copy(src, runtime::impl_access::get_impl(dest));
#endif
}

template <class SrcT, int SrcDim, access_mode SrcMode, target SrcTgt, class DestT, int DestDim,
          access_mode DestMode, target DestTgt, class>
void handler::copy(accessor<SrcT, SrcDim, SrcMode, SrcTgt> const& src,
                   accessor<DestT, DestDim, DestMode, DestTgt> const& dest) {
    static_assert(std::is_same_v<SrcT, DestT>, "Type mismatch");
    static_assert(SrcDim == DestDim, "Dimension mismatch");
    static_assert(detail::is_readable(SrcMode), "Source is not writable");
    static_assert(detail::is_writable(DestMode), "Destination is not writable");
    static_assert(DestTgt == target::device, "Non-device copy is not supported");
    static_assert(SrcTgt == target::device, "Non-device copy is not supported");

#ifdef __SYCL_DEVICE_ONLY__
    (void)src;
    (void)dest;
#else
    impl_->copy(runtime::impl_access::get_impl(src), runtime::impl_access::get_impl(dest));
#endif
}

template <class T, int Dim, access_mode Mode, target Tgt>
void handler::fill(accessor<T, Dim, Mode, Tgt> const& dest, T const& src) {
    static_assert(Tgt == target::device, "Non-device fill is not supported");
    static_assert(detail::is_writable(Mode), "Accessor is not writable");

    auto& h = *this;
    h.parallel_for(dest.get_range(), [=](id<Dim> id) {
        dest[id] = src;
    });
}

inline event handler::finalize() {
    return runtime::impl_access::from_impl<event>(impl_->finalize());
}

namespace detail {
template <class T>
auto constexpr has_impl_v = decltype(runtime::impl_access::has_impl(std::declval<T>()))::value;
}  // namespace detail

template <class... Args>
inline void handler::__bind(Args const&... args) {
    impl_->reserve_binds(sizeof...(Args));
    impl_->begin_binds();
    try {
        bind_args<true, 0>(args...);
        impl_->end_pre_binds();
        bind_args<false, 0>(args...);
    } catch (...) {
        impl_->end_binds();
        throw;
    }
    impl_->end_binds();
}

template <bool, size_t>
inline void handler::bind_args() {}

template <bool Pre, size_t Idx, class Arg, class... Args>
inline void handler::bind_args(Arg const& arg, Args const&... args) {
    bind_arg<Pre, Idx>(arg);
    bind_args<Pre, Idx + 1>(args...);
}

template <bool Pre, size_t Idx, class Arg>
inline void handler::bind_arg(Arg const& arg) {
    if constexpr (detail::has_impl_v<Arg>) {
        if constexpr (Pre) {
            impl_->pre_bind(Idx, runtime::impl_access::get_impl(arg));
        } else {
            impl_->bind(Idx, runtime::impl_access::get_impl(arg));
        }
    } else {
        if constexpr (Pre) {
            impl_->pre_bind(Idx, std::addressof(arg), sizeof(arg));
        } else {
            impl_->bind(Idx, std::addressof(arg), sizeof(arg));
        }
    }
}

template <class KernelName, class KernelType>
inline void handler::do_bind(KernelType& fn) {
#if defined(__SYCL_DEVICE_ONLY__) || !defined(__cham_sycl_kernel_descs_DEFINED)
    (void)fn;
    throw std::runtime_error("BUG: no kernel descriptor is found");
#else
    static constinit auto const desc = ::__cham_sycl_kernel_descs::get<KernelName>();
    static constinit auto const accessors = desc.accessors();

    impl_->reserve_binds(accessors.size() + 1);
    impl_->begin_binds();
    try {
        auto* fn_ptr =
            const_cast<std::add_pointer_t<std::remove_cvref_t<KernelType>>>(std::addressof(fn));

        bind_arg<true, 0>(fn);
        do_bind_accessors<true, 1, accessors>(fn_ptr);
        impl_->end_pre_binds();
        bind_arg<false, 0>(fn);
        do_bind_accessors<false, 1, accessors>(fn_ptr);
    } catch (...) {
        impl_->end_binds();
        throw;
    }
    impl_->end_binds();
#endif
}

template <bool Pre, size_t I, auto List, class KernelType>
inline void handler::do_bind_accessors(KernelType* fn) {
    if constexpr (!List.empty()) {
        auto* acc = List.head().interpret(reinterpret_cast<std::byte*>(fn));

        if constexpr (Pre) {
            acc->into_device();
        }
        bind_arg<Pre, I>(*acc);

        if constexpr (List.size() > 1) {
            do_bind_accessors<Pre, I + 1, List.tail()>(fn);
        }
    }
}

CHARM_SYCL_END_NAMESPACE
