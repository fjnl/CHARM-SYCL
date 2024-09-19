#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct buffer : refcnt_base {
    virtual ~buffer() = default;

    virtual void write_back() = 0;

    virtual range<3> get_range() const = 0;

    virtual size_t byte_size() const = 0;

    virtual void set_write_back(bool on) = 0;

    virtual void set_final_pointer(void* ptr) = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
