#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct accessor;
struct buffer;
struct context;
struct device;
struct event_barrier;
struct event;
struct handler;
struct host_accessor;
struct local_accessor;
struct platform;
struct property_list;
struct queue;

using accessor_ptr = intrusive_ptr<accessor>;
using buffer_ptr = intrusive_ptr<buffer>;
using context_ptr = intrusive_ptr<context>;
using device_ptr = intrusive_ptr<device>;
using event_ptr = intrusive_ptr<event>;
using handler_ptr = intrusive_ptr<handler>;
using host_accessor_ptr = intrusive_ptr<host_accessor>;
using local_accessor_ptr = intrusive_ptr<local_accessor>;
using platform_ptr = intrusive_ptr<platform>;
using queue_ptr = intrusive_ptr<queue>;

accessor_ptr make_accessor(handler_ptr const& handler, buffer_ptr const& buf, range<3> range,
                           id<3> offset, access_mode mode);

local_accessor_ptr make_local_accessor(handler_ptr const& handler, int dim, size_t elem_size,
                                       size_t align, range<3> range);

host_accessor_ptr make_host_accessor(buffer_ptr const& buf, range<3> range, id<3> offset,
                                     access_mode mode);

// TODO: Use Allocator
buffer_ptr make_buffer(void* init_ptr, size_t elemsize, range<3> const& rng);

handler_ptr make_handler(queue_ptr const&);

queue_ptr make_queue(context_ptr const&, device_ptr const&,
                     sycl::property::queue::enable_profiling const*);

vec<platform_ptr> get_platforms();

template <class... Ps>
std::unique_ptr<property_list> make_property_list(Ps&&... props);

extern "C" CHARM_SYCL_HOST_INLINE void* __charm_sycl_local_memory_base();

extern "C" CHARM_SYCL_HOST_INLINE void __charm_sycl_group_barrier(void const* g,
                                                                  memory_scope fence_scope);

extern "C" CHARM_SYCL_HOST_INLINE void __charm_sycl_assume(bool);

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
