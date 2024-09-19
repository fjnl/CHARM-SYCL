#pragma once

#include <vector>
#include <charm/sycl/config.hpp>
#include <charm/sycl/access_mode.hpp>
#include <charm/sycl/runtime/intrusive_ptr.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {
template <class T>
struct allocator;

struct impl_access {
    template <class T>
    static auto& get_impl(T& x) {
        return x.impl_;
    }

    template <class T>
    static auto const& get_impl(T const& x) {
        return x.impl_;
    }

    template <class T, class U>
    static T from_impl(intrusive_ptr<U> const& impl) {
        return T(impl);
    }

    template <class T, class U>
    static std::vector<T> from_impl(vec<intrusive_ptr<U>> const& impls) {
        std::vector<T> ret;
        for (auto const& impl : impls) {
            ret.push_back(from_impl<T>(impl));
        }
        return ret;
    }

    template <class T>
    static auto has_impl(T&& x) -> decltype(std::addressof(x.impl_), std::true_type());

    static auto has_impl(...) -> std::false_type;

    template <class T, class... Args>
    static auto make(Args&&... args) {
        return T(std::forward<Args>(args)...);
    }

    template <class T, class... Args>
    static decltype(auto) get_queue(T& obj) {
        return obj.q_;
    }
};

}  // namespace runtime

namespace detail {

template <class Derived>
struct common_ref_ops {
    friend bool operator==(Derived const& lhs, Derived const& rhs) {
        runtime::impl_access::get_impl(lhs) == runtime::impl_access::get_impl(rhs);
    }

    friend bool operator!=(Derived const& lhs, Derived const& rhs) {
        runtime::impl_access::get_impl(lhs) != runtime::impl_access::get_impl(rhs);
    }

    void swap(Derived& other) {
        std::swap(runtime::impl_access::get_impl(*this), runtime::impl_access::get_impl(other));
    }
};

}  // namespace detail

template <class T>
using buffer_allocator = runtime::allocator<T>;

template <class DataT, int Dimensions = 1,
          access_mode AccessMode =
              (std::is_const_v<DataT> ? access_mode::read : access_mode::read_write),
          target AccessTarget = target::device>
struct accessor;

template <typename DataT, int Dimensions = 1>
class local_accessor;

namespace detail {
template <class DataT, int Dimensions, access_mode AccessMode>
struct device_accessor;
}

template <class DataT, int Dimensions, access_mode AccessMode>
struct host_accessor;

template <class T, int Dimensions = 1, class AllocatorT = buffer_allocator<T>>
struct buffer;

struct context;

struct device;

struct event;

struct handler;

struct platform;

struct queue;

struct property_list;

template <int Dimensions = 1>
struct id;

template <int Dimensions = 1>
struct range;

template <int Dimensions = 1>
struct nd_range;

template <int Dimensions = 1, bool UseOffset = false>
struct item;

template <int Dimensions = 1>
struct nd_item;

template <int Dimensions>
struct group;

template <int Dimensions>
struct h_item;

namespace detail {
template <class DataT, int NumElements>
inline constexpr size_t vec_size = sizeof(DataT) * (NumElements == 3 ? 4 : NumElements);
}  // namespace detail

template <class DataT, int NumElements>
struct alignas(detail::vec_size<DataT, NumElements>) vec;

namespace detail {
struct unnamed_kernel;
}

using async_handler = std::function<void(std::exception_ptr)>;

enum class errc : int {
    success = 0,
    runtime,
    kernel,
    accessor,
    nd_range,
    event,
    kernel_argument,
    build,
    invalid,
    memory_allocation,
    platform,
    profiling,
    feature_not_supported,
    kernel_not_supported,
    backend_mismatch,
    unimplemented = 100,
};

[[noreturn]] inline void unimplemented();

[[noreturn]] inline void throw_error(errc e, char const* what);

enum class memory_scope { work_item, sub_group, work_group, device, system };

template <class T>
struct is_group : std::false_type {};

template <class T>
inline constexpr bool is_group_v = is_group<T>::value;

template <class Group>
void group_barrier(Group& g, memory_scope fence_scope = Group::fence_scope);

namespace detail {

inline sycl::id<3> extend(sycl::id<1> const&);
inline sycl::id<3> extend(sycl::id<2> const&);
inline sycl::id<3> extend(sycl::id<3> const&);

inline sycl::range<3> extend(sycl::range<1> const&);
inline sycl::range<3> extend(sycl::range<2> const&);
inline sycl::range<3> extend(sycl::range<3> const&);

inline sycl::nd_range<3> extend(sycl::nd_range<1> const&);
inline sycl::nd_range<3> extend(sycl::nd_range<2> const&);
inline sycl::nd_range<3> extend(sycl::nd_range<3> const&);

template <int _Dim>
inline sycl::range<_Dim> shrink(sycl::range<3> const& r);

template <int _Dim>
inline sycl::range<_Dim> shrink(sycl::id<3> const& id);

template <int Dimensions, class Range, class Id>
inline size_t linear_id(Range const& range, Id const& id);

template <int Dimensions, class Range>
inline auto linear_range(Range const& range);

template <int D>
sycl::nd_item<D> make_nd_item();

template <int D>
sycl::nd_item<D> make_nd_item(range<D> const& group_range, range<D> const& local_range,
                              id<D> const& group_id, id<D> const& local_id);

}  // namespace detail

template <class... Ts>
inline constexpr bool not_supported = false;

#ifdef __SYCL_DEVICE_ONLY__
#    define CHARM_SYCL_HOST_INLINE
#else
#    define CHARM_SYCL_HOST_INLINE inline __attribute__((always_inline))
#endif

namespace property::queue {
struct enable_profiling;
}

CHARM_SYCL_END_NAMESPACE
