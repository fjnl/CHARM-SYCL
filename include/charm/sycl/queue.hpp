#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace property::queue {
struct enable_profiling {};
}  // namespace property::queue

struct queue {
    explicit inline queue(const property_list& propList = {});

    explicit inline queue(const async_handler& asyncHandler,
                          const property_list& propList = {});

    template <class DeviceSelector>
    explicit inline queue(const DeviceSelector& deviceSelector,
                          const property_list& propList = {});

    template <typename DeviceSelector>
    explicit inline queue(const DeviceSelector& deviceSelector,
                          const async_handler& asyncHandler,
                          const property_list& propList = {});

    explicit inline queue(const device& syclDevice, const property_list& propList = {});

    explicit inline queue(const device& syclDevice, const async_handler& asyncHandler,
                          const property_list& propList = {});

    template <typename DeviceSelector>
    explicit inline queue(const context& syclContext, const DeviceSelector& deviceSelector,
                          const property_list& propList = {});

    template <typename DeviceSelector>
    explicit inline queue(const context& syclContext, const DeviceSelector& deviceSelector,
                          const async_handler& asyncHandler,
                          const property_list& propList = {});

    explicit inline queue(const context& syclContext, const device& syclDevice,
                          const property_list& propList = {});

    explicit inline queue(const context& syclContext, const device& syclDevice,
                          const async_handler& asyncHandler,
                          const property_list& propList = {});

    inline backend get_backend() const noexcept;

    inline context get_context() const;

    inline device get_device() const;

    inline bool is_in_order() const {
        return false;
    }

    // template <typename Param>
    // typename Param::return_type get_info() const;

    // template <typename Param>
    // typename Param::return_type get_backend_info() const;

    template <typename T>
    inline event submit(T cgf);

    // template <typename T>
    // event submit(T cgf, const queue& secondaryQueue);

    inline void wait();

    inline void wait_and_throw();

    inline void throw_asynchronous();

    /* -- convenience shortcuts -- */

    // template <typename KernelName, typename KernelType>
    // event single_task(const KernelType& kernelFunc);

    // template <typename KernelName, typename KernelType>
    // event single_task(event depEvent, const KernelType& kernelFunc);

    // template <typename KernelName, typename KernelType>
    // event single_task(const std::vector<event>& depEvents, const KernelType& kernelFunc);

    // // Parameter pack acts as-if: Reductions&&... reductions, const KernelType &kernelFunc
    // template <typename KernelName, int Dims, typename... Rest>
    // event parallel_for(range<Dims> numWorkItems, Rest&&... rest);

    // // Parameter pack acts as-if: Reductions&&... reductions, const KernelType &kernelFunc
    // template <typename KernelName, int Dims, typename... Rest>
    // event parallel_for(range<Dims> numWorkItems, event depEvent, Rest&&... rest);

    // // Parameter pack acts as-if: Reductions&&... reductions, const KernelType &kernelFunc
    // template <typename KernelName, int Dims, typename... Rest>
    // event parallel_for(range<Dims> numWorkItems, const std::vector<event>& depEvents,
    //                    Rest&&... rest);

    // // Parameter pack acts as-if: Reductions&&... reductions, const KernelType &kernelFunc
    // template <typename KernelName, int Dims, typename... Rest>
    // event parallel_for(nd_range<Dims> executionRange, Rest&&... rest);

    // // Parameter pack acts as-if: Reductions&&... reductions, const KernelType &kernelFunc
    // template <typename KernelName, int Dims, typename... Rest>
    // event parallel_for(nd_range<Dims> executionRange, event depEvent, Rest&&... rest);

    // // Parameter pack acts as-if: Reductions&&... reductions, const KernelType &kernelFunc
    // template <typename KernelName, int Dims, typename... Rest>
    // event parallel_for(nd_range<Dims> executionRange, const std::vector<event>& depEvents,
    //                    Rest&&... rest);

    /// Placeholder accessor shortcuts

    // Explicit copy functions

    // template <typename SrcT, int SrcDims, access_mode SrcMode, target SrcTgt,
    //           access::placeholder IsPlaceholder, typename DestT>
    // event copy(accessor<SrcT, SrcDims, SrcMode, SrcTgt, IsPlaceholder> src,
    //            std::shared_ptr<DestT> dest);

    // template <typename SrcT, typename DestT, int DestDims, access_mode DestMode, target
    // DestTgt,
    //           access::placeholder IsPlaceholder>
    // event copy(std::shared_ptr<SrcT> src,
    //            accessor<DestT, DestDims, DestMode, DestTgt, IsPlaceholder> dest);

    // template <typename SrcT, int SrcDims, access_mode SrcMode, target SrcTgt,
    //           access::placeholder IsPlaceholder, typename DestT>
    // event copy(accessor<SrcT, SrcDims, SrcMode, SrcTgt, IsPlaceholder> src, DestT* dest);

    // template <typename SrcT, typename DestT, int DestDims, access_mode DestMode, target
    // DestTgt,
    //           access::placeholder IsPlaceholder>
    // event copy(const SrcT* src,
    //            accessor<DestT, DestDims, DestMode, DestTgt, IsPlaceholder> dest);

    // template <typename SrcT, int SrcDims, access_mode SrcMode, target SrcTgt,
    //           access::placeholder IsSrcPlaceholder, typename DestT, int DestDims,
    //           access_mode DestMode, target DestTgt, access::placeholder IsDestPlaceholder>
    // event copy(accessor<SrcT, SrcDims, SrcMode, SrcTgt, IsSrcPlaceholder> src,
    //            accessor<DestT, DestDims, DestMode, DestTgt, IsDestPlaceholder> dest);

    // template <typename T, int Dims, access_mode Mode, target Tgt,
    //           access::placeholder IsPlaceholder>
    // event update_host(accessor<T, Dim, Mode, Tgt, IsPlaceholder> acc);

    // template <typename T, int Dims, access_mode Mode, target Tgt,
    //           access::placeholder IsPlaceholder>
    // event fill(accessor<T, Dims, Mode, Tgt, IsPlaceholder> dest, const T& src);
private:
    friend struct runtime::impl_access;

    explicit queue(runtime::queue_ptr const& impl) : impl_(impl) {}

    runtime::queue_ptr impl_;
};

CHARM_SYCL_END_NAMESPACE
