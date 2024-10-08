#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct accessor : refcnt_base {
    virtual ~accessor() = default;

    virtual range<3> get_range() const = 0;

    virtual id<3> get_offset() const = 0;

    virtual void* get_pointer() = 0;

    virtual buffer_ptr get_buffer() = 0;
};

struct host_accessor : refcnt_base {
    virtual ~host_accessor() = default;

    virtual range<3> get_range() const = 0;

    virtual id<3> get_offset() const = 0;

    virtual void* get_pointer() = 0;

    virtual buffer_ptr get_buffer() = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
