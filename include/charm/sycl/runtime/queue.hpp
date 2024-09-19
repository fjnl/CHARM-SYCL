#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct queue : refcnt_base {
    virtual ~queue() = default;

    virtual backend get_backend() const noexcept = 0;

    virtual device_ptr get_device() const = 0;

    virtual context_ptr get_context() const = 0;

    virtual void add(event_ptr const&) = 0;

    virtual void wait() = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
