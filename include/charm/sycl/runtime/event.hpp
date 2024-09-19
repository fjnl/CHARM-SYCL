#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct event : refcnt_base {
    virtual ~event() = default;

    virtual event_barrier* create_barrier() = 0;
    virtual void release_barrier(event_barrier*) = 0;

    virtual uint64_t profiling_command_submit() = 0;
    virtual uint64_t profiling_command_start() = 0;
    virtual uint64_t profiling_command_end() = 0;
};

struct event_barrier {
    virtual ~event_barrier() = default;

    virtual void add(event&) = 0;

    virtual void wait() = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
