#pragma once

#include <charm/sycl.hpp>
#include <charm/sycl/fnv1a.hpp>

#if !defined(CHARM_SYCL_HAS_SYCL_UNIQUE_STABLE_NAME)
#    if defined(__clang__) && __clang_major__ >= 13 && defined(__has_builtin)
#        if __has_builtin(__builtin_sycl_unique_stable_name)
#            define CHARM_SYCL_HAS_SYCL_UNIQUE_STABLE_NAME 1
#        else
#            define CHARM_SYCL_HAS_SYCL_UNIQUE_STABLE_NAME 0
#        endif
#    endif
#endif

#if defined(__SYCL_DEVICE_ONLY__) && !CHARM_SYCL_HAS_SYCL_UNIQUE_STABLE_NAME
#    error __builtin_sycl_unique_stable_name() is not supported by the compiler.
#endif

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

struct has_bind_ops_wrapper {
    template <class K, class H>
    auto operator()(K const& k, H& h) -> decltype(K::__do_binds(h, k), std::true_type());

    auto operator()(...) -> std::false_type;
};

template <class K, class H>
constexpr bool has_bind_ops_v =
    decltype(has_bind_ops_wrapper()(std::declval<K const&>(), std::declval<H&>()))::value;

}  // namespace detail

inline handler::handler(queue const& q)
    : impl_(runtime::make_handler(runtime::impl_access::get_impl(q))) {}

template <class KernelName, int Dimensions, class KernelType>
inline void handler::parallel_for(range<Dimensions> const& range, KernelType const& kernel) {
#if !CHARM_SYCL_HAS_SYCL_UNIQUE_STABLE_NAME
    (void)range;
    (void)kernel;
    std::terminate();
#else
    constexpr char const* type_name = __builtin_sycl_unique_stable_name(KernelType);

    auto const type_len = std::strlen(type_name);
    auto const name_len = 3 + type_len;
    auto* name = reinterpret_cast<char*>(alloca(name_len + 1));
    memcpy(name, "_s_", 3);
    memcpy(name + 3, type_name, type_len);
    name[name_len] = '\0';

    auto const h = detail::fnv1a(name, name_len);
    impl_->parallel_for(detail::extend(range), name, h);

    if constexpr (detail::has_bind_ops_v<KernelType, handler>) {
        KernelType::__do_binds(*this, kernel);
    }

#    ifdef __SYCL_DEVICE_ONLY__
    // Dummy invocations are required to keep the function body in the AST.
    if constexpr (Dimensions == 1) {
        (void)kernel(detail::make_item(sycl::range<Dimensions>(1), sycl::id<Dimensions>()));
    } else if constexpr (Dimensions == 2) {
        (void)kernel(detail::make_item(sycl::range<Dimensions>(1, 1), sycl::id<Dimensions>()));
    } else {
        (void)kernel(
            detail::make_item(sycl::range<Dimensions>(1, 1, 1), sycl::id<Dimensions>()));
    }
#    endif
#endif
}

template <class KernelName, int Dimensions, class KernelType>
inline void handler::parallel_for(range<Dimensions> const& range, id<Dimensions> const& offset,
                                  KernelType const& kernel) {
#if !CHARM_SYCL_HAS_SYCL_UNIQUE_STABLE_NAME
    (void)offset;
    (void)range;
    (void)kernel;
    std::terminate();
#else
    constexpr char const* type_name = __builtin_sycl_unique_stable_name(KernelType);

    auto const type_len = std::strlen(type_name);
    auto const name_len = 3 + type_len;
    auto* name = reinterpret_cast<char*>(alloca(name_len + 1));
    memcpy(name, "_s_", 3);
    memcpy(name + 3, type_name, type_len);
    name[name_len] = '\0';

    auto const h = detail::fnv1a(name, name_len);
    impl_->parallel_for(detail::extend(range), detail::extend(offset), name, h);

    if constexpr (detail::has_bind_ops_v<KernelType, handler>) {
        KernelType::__do_binds(*this, kernel);
    }

#    ifdef __SYCL_DEVICE_ONLY__
    // Dummy invocations are required to keep the function body in the AST.
    if constexpr (Dimensions == 1) {
        (void)kernel(detail::make_item(sycl::range<Dimensions>(1), sycl::id<Dimensions>()));
    } else if constexpr (Dimensions == 2) {
        (void)kernel(detail::make_item(sycl::range<Dimensions>(1, 1), sycl::id<Dimensions>()));
    } else {
        (void)kernel(
            detail::make_item(sycl::range<Dimensions>(1, 1, 1), sycl::id<Dimensions>()));
    }
#    endif
#endif
}

template <class KernelName, class KernelType>
inline void handler::single_task(KernelType const& kernel) {
#if !CHARM_SYCL_HAS_SYCL_UNIQUE_STABLE_NAME
    (void)kernel;
    std::terminate();
#else
    constexpr char const* type_name = __builtin_sycl_unique_stable_name(KernelType);

    auto const type_len = std::strlen(type_name);
    auto const name_len = 3 + type_len;
    auto* name = reinterpret_cast<char*>(alloca(name_len + 1));
    memcpy(name, "_s_", 3);
    memcpy(name + 3, type_name, type_len);
    name[name_len] = '\0';

    auto const h = detail::fnv1a(name, name_len);
    impl_->single_task(name, h);

    if constexpr (detail::has_bind_ops_v<KernelType, handler>) {
        KernelType::__do_binds(*this, kernel);
    }

#    ifdef __SYCL_DEVICE_ONLY__
    // Dummy invocation is required to keep the function body in the AST.
    (void)kernel();
#    endif
#endif
}

template <class KernelName, int Dimensions, class KernelType>
inline void handler::parallel_for(nd_range<Dimensions> const& ndr, KernelType const& kernel) {
#if !CHARM_SYCL_HAS_SYCL_UNIQUE_STABLE_NAME
    (void)ndr;
    (void)kernel;
    std::terminate();
#else
    constexpr char const* type_name = __builtin_sycl_unique_stable_name(KernelType);

    auto const type_len = std::strlen(type_name);
    auto const name_len = 3 + type_len;
    auto* name = reinterpret_cast<char*>(alloca(name_len + 1));
    memcpy(name, "_s_", 3);
    memcpy(name + 3, type_name, type_len);
    name[name_len] = '\0';

    auto const h = detail::fnv1a(name, name_len);
    impl_->parallel_for(detail::extend(ndr), name, h);

    if constexpr (detail::has_bind_ops_v<KernelType, handler>) {
        KernelType::__do_binds(*this, kernel);
    }

#    ifdef __SYCL_DEVICE_ONLY__
    // Dummy invocations are required to keep the function body in the AST.
    (void)kernel(detail::make_nd_item<Dimensions>());
#    endif
#endif
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

template <class Obj, class Arg>
void __bind_arg(Obj& obj, Arg const& arg) {
    if constexpr (has_impl_v<Arg>) {
        obj->bind(runtime::impl_access::get_impl(arg));
    } else if constexpr (std::is_trivially_copyable_v<Arg>) {
        obj->bind(std::addressof(arg), sizeof(arg));
    } else {
        static_assert(not_supported<Arg>, "not a device-copyable type");
    }
}

}  // namespace detail

template <class... Args>
inline void handler::__bind(Args const&... args) {
    impl_->begin_binds();
    try {
        __bind_args(args...);
    } catch (...) {
        impl_->end_binds();
        throw;
    }
    impl_->end_binds();
}

inline void handler::__bind_args() {}

template <class Arg, class... Args>
inline void handler::__bind_args(Arg const& arg, Args const&... args) {
    detail::__bind_arg(impl_, arg);

    __bind_args(args...);
}

CHARM_SYCL_END_NAMESPACE
