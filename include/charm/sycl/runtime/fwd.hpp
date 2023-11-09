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

std::shared_ptr<accessor> make_accessor(std::shared_ptr<handler> const& handler,
                                        std::shared_ptr<buffer> const& buf, range<3> range,
                                        id<3> offset, access_mode mode,
                                        property_list const* props);

std::shared_ptr<local_accessor> make_local_accessor(std::shared_ptr<handler> const& handler,
                                                    int dim, size_t elem_size, size_t align,
                                                    range<3> range, property_list const* props);

std::shared_ptr<host_accessor> make_host_accessor(std::shared_ptr<buffer> const& buf,
                                                  range<3> range, id<3> offset,
                                                  access_mode mode, property_list const* props);

// TODO: Use Allocator
std::shared_ptr<buffer> make_buffer(void* init_ptr, std::shared_ptr<void> const& sp,
                                    size_t elemsize, range<3> const& rng,
                                    property_list const* props);

std::shared_ptr<handler> make_handler(std::shared_ptr<queue> const&);

std::shared_ptr<queue> make_queue(std::shared_ptr<context> const&,
                                  std::shared_ptr<device> const&, async_handler const&,
                                  property_list const*);

std::vector<std::shared_ptr<platform>> get_platforms();

template <class... Ps>
std::unique_ptr<property_list> make_property_list(Ps&&... props);

extern "C" CHARM_SYCL_HOST_INLINE void* __charm_sycl_local_memory_base();

extern "C" CHARM_SYCL_HOST_INLINE void __charm_sycl_group_barrier(void const* g,
                                                                  memory_scope fence_scope);

extern "C" CHARM_SYCL_HOST_INLINE void __charm_sycl_assume(bool);

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
