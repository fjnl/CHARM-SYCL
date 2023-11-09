#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

struct handler {
    friend struct queue;

    template <class KernelName = detail::unnamed_kernel, class KernelType>
    inline void single_task(KernelType const& kernel);

    template <class KernelName = detail::unnamed_kernel, int Dimensions, class KernelType>
    inline void parallel_for(range<Dimensions> const& range, KernelType const& kernel);

    template <class KernelName = detail::unnamed_kernel, int Dimensions, class KernelType>
    inline void parallel_for(range<Dimensions> const& range, id<Dimensions> const& offset,
                             KernelType const& kernel);

    template <class KernelName = detail::unnamed_kernel, int Dimensions, class KernelType>
    inline void parallel_for(nd_range<Dimensions> const& range, KernelType const& kernel);

    template <class SrcT, int SrcDim, access_mode SrcMode, target SrcTgt, class DestT>
    void copy(accessor<SrcT, SrcDim, SrcMode, SrcTgt> src, DestT* dest);

    template <class SrcT, class DestT, int DestDim, access_mode DestMode, target DestTgt>
    void copy(const SrcT* src, accessor<DestT, DestDim, DestMode, DestTgt> dest);

    template <class SrcT, int SrcDim, access_mode SrcMode, target SrcTgt, class DestT,
              int DestDim, access_mode DestMode, target DestTgt,
              class = std::enable_if_t<std::is_same_v<SrcT, DestT>>>
    void copy(accessor<SrcT, SrcDim, SrcMode, SrcTgt> const& src,
              accessor<DestT, DestDim, DestMode, DestTgt> const& dest);

    template <class T, int Dim, access_mode Mode, target Tgt>
    void fill(accessor<T, Dim, Mode, Tgt> const& dest, const T& src);

    template <class T, class U, int Dim, access_mode Mode, target Tgt,
              class = std::enable_if<std::is_convertible_v<U, T>>>
    void fill(accessor<T, Dim, Mode, Tgt> const& dest, const U& src) {
        fill(dest, static_cast<T>(src));
    }

    /* internal API */
    template <class... Args>
    inline void __bind(Args const&...);

private:
    friend struct runtime::impl_access;

    explicit inline handler(queue const& q);

    explicit handler(std::shared_ptr<runtime::handler> const& impl) : impl_(impl) {}

    inline void __bind_args();

    template <class Arg, class... Args>
    inline void __bind_args(Arg const&, Args const&...);

    inline event finalize();

    std::shared_ptr<runtime::handler> impl_;
};

CHARM_SYCL_END_NAMESPACE
