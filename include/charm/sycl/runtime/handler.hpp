#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct handler : refcnt_base {
    virtual ~handler() = default;

    virtual void depends_on(event_ptr const&) = 0;

    virtual void single_task(char const* name, uint32_t hash) = 0;

    virtual void parallel_for(range<3> const& range, char const* name, uint32_t hash) = 0;

    virtual void parallel_for(nd_range<3> const& ndr, char const* name, uint32_t hash) = 0;

    virtual void set_desc(void const* desc) = 0;

    virtual event_ptr finalize() = 0;

    virtual void reserve_binds(size_t n) = 0;
    virtual void begin_binds() = 0;
    virtual void end_pre_binds() = 0;
    virtual void end_binds() = 0;

    virtual void pre_bind(size_t idx, accessor_ptr const& acc) = 0;
    virtual void pre_bind(size_t idx, local_accessor_ptr const& acc) = 0;
    virtual void pre_bind(size_t idx, void const* addr, size_t size) = 0;

    virtual void bind(size_t idx, accessor_ptr const& acc) = 0;
    virtual void bind(size_t, local_accessor_ptr const& acc) = 0;
    virtual void bind(size_t, void const* addr, size_t size) = 0;

    virtual void copy(accessor_ptr const& src, accessor_ptr const& dest) = 0;

    virtual void copy(accessor_ptr const& src, void* dest) = 0;

    virtual void copy(void const* src, accessor_ptr const& dest) = 0;

    virtual void fill_zero(accessor_ptr const& src, size_t len_byte) = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
