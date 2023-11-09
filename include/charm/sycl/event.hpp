#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace info::event_profiling {

struct command_submit {
    using return_type = uint64_t;
};

struct command_start {
    using return_type = uint64_t;
};

struct command_end {
    using return_type = uint64_t;
};

}  // namespace info::event_profiling

struct event {
    event() = default;

    inline void wait();

    template <typename Param>
    inline typename Param::return_type get_profiling_info() const {
        return get_profiling_info(Param());
    }

private:
    friend struct runtime::impl_access;

    inline explicit event(std::shared_ptr<runtime::event> const& impl) : impl_(impl) {}

    inline info::event_profiling::command_submit::return_type get_profiling_info(
        info::event_profiling::command_submit) const;

    inline info::event_profiling::command_start::return_type get_profiling_info(
        info::event_profiling::command_start) const;

    inline info::event_profiling::command_end::return_type get_profiling_info(
        info::event_profiling::command_end) const;

    std::shared_ptr<runtime::event> impl_;
};

CHARM_SYCL_END_NAMESPACE
