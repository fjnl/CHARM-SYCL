#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

struct handler {
    friend struct queue;

    template <class KernelName = detail::unnamed_kernel, class KernelType>
    inline void single_task(KernelType const& kernel);

    template <class KernelName = detail::unnamed_kernel, int Dimensions, class... Rest>
    inline void parallel_for(range<Dimensions> const& range, Rest&&... rest);

    template <class KernelName = detail::unnamed_kernel, int Dimensions, class KernelType>
    inline void parallel_for(nd_range<Dimensions> const& range, KernelType const&);

    // template <class KernelName = detail::unnamed_kernel, int Dimensions, class KernelType,
    //           class... Rest>
    // inline void parallel_for(nd_range<Dimensions> const& range, KernelType const&,
    //                          Rest&&... rest);

    template <class KernelName = detail::unnamed_kernel, class WorkgroupFunctionType,
              int Dimensions>
    inline void parallel_for_work_group(range<Dimensions> const& numWorkGroups,
                                        range<Dimensions> const& workGroupSize,
                                        WorkgroupFunctionType const& fn);

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

    inline void depends_on(event ev);

    inline void depends_on(std::vector<event> const& events) {
        for (auto const& ev : events) {
            depends_on(ev);
        }
    }

    /* internal API */
    template <class... Args>
    inline void __bind(Args const&...);

private:
    friend struct runtime::impl_access;

    template <class DataT, int Dimensions, class AllocatorT, class BinaryOperation>
    friend auto reduction(buffer<DataT, Dimensions, AllocatorT> vars, handler& cgh,
                          std::optional<DataT>, BinaryOperation&& combiner,
                          property_list const&);

    explicit inline handler(queue const& q);

    explicit handler(runtime::handler_ptr const& impl, runtime::queue_ptr const& q)
        : q_(q), impl_(impl) {}

    template <class KernelName, int Dimensions, class KernelType>
    inline void parallel_for_1(range<Dimensions> const& range, KernelType& kernel);

    template <class KernelName, int Dimensions, class KernelType>
    inline void parallel_for_2(range<Dimensions> const& range, id<Dimensions> const& offset,
                               KernelType& kernel);

    template <class KernelName, int Dimensions, class KernelType>
    inline void parallel_for_3(nd_range<Dimensions> const& range, KernelType const&);

    template <class KernelName, int Dimensions, class KernelType, size_t... I,
              class... Reducers>
    inline void parallel_for_reduce(range<Dimensions> const& range, KernelType& fn,
                                    std::index_sequence<I...>, Reducers&... reducers);

    template <class KernelName, int Dimensions, class KernelType>
    inline void parallel_for_(range<Dimensions> const& range, KernelType const& kernel);

    template <class KernelName, int Dimensions, class KernelType>
    inline void parallel_for_(nd_range<Dimensions> const& range, KernelType const& kernel);

    template <class KernelName, class KernelType>
    inline void single_task_(KernelType const& kernel);

    template <class KernelName, class KernelType>
    inline void do_bind(KernelType& fn);

    template <bool Pre, size_t I, auto List, class KernelType>
    inline void do_bind_accessors(KernelType* fn);

    template <bool Pre, size_t Idx, class Arg>
    inline void bind_arg(Arg const&);

    template <bool, size_t>
    inline void bind_args();

    template <bool Pre, size_t Idx, class Arg, class... Args>
    inline void bind_args(Arg const&, Args const&...);

    inline event finalize();

    runtime::queue_ptr q_;
    runtime::handler_ptr impl_;
};

namespace runtime {

template <size_t Len>
struct k_name {
    constexpr explicit k_name(char const* name) {
        name_[0] = '_';
        name_[1] = 's';
        name_[2] = '_';
        for (size_t i = 0; i < Len; i++) {
            name_[3 + i] = name[i];
        }
        name_[3 + Len] = 0;
    }

    constexpr char const* get() const {
        return name_;
    }

    constexpr size_t size() const {
        return Len;
    }

private:
    char name_[3 + Len + 1];
};

template <class Name>
inline char const* __charm_sycl_kernel_name(size_t& name_len) {
#if __has_builtin(__builtin_sycl_unique_stable_name)
    static constinit k_name<__builtin_strlen(
        __builtin_sycl_unique_stable_name(std::remove_cvref_t<Name>))>
        name(__builtin_sycl_unique_stable_name(std::remove_cvref_t<Name>));
    name_len = name.size();
    return name.get();
#else
    name_len = 0;
    return nullptr;
#endif
}

void __charm_sycl_kernel(void const*);

size_t __charm_sycl_group_range1();
size_t __charm_sycl_group_id1();
size_t __charm_sycl_group_range2();
size_t __charm_sycl_group_id2();
size_t __charm_sycl_group_range3();
size_t __charm_sycl_group_id3();

size_t __charm_sycl_local_range1();
size_t __charm_sycl_local_id1();
size_t __charm_sycl_local_range2();
size_t __charm_sycl_local_id2();
size_t __charm_sycl_local_range3();
size_t __charm_sycl_local_id3();

size_t __charm_sycl_parallel_iter1_begin();
bool __charm_sycl_parallel_iter1_cond(size_t, size_t);
size_t __charm_sycl_parallel_iter1_step(size_t);

size_t __charm_sycl_parallel_iter2_begin();
bool __charm_sycl_parallel_iter2_cond(size_t, size_t);
size_t __charm_sycl_parallel_iter2_step(size_t);

size_t __charm_sycl_parallel_iter3_begin();
bool __charm_sycl_parallel_iter3_cond(size_t, size_t);
size_t __charm_sycl_parallel_iter3_step(size_t);

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
