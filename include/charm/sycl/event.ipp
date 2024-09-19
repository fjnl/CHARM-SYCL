#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

void event::wait() {
    auto barrier = std::unique_ptr<runtime::event_barrier>(impl_->create_barrier());
    barrier->add(*impl_);
    barrier->wait();
}

info::event_profiling::command_submit::return_type event::get_profiling_info(
    info::event_profiling::command_submit) const {
    return impl_->profiling_command_submit();
}

info::event_profiling::command_start::return_type event::get_profiling_info(
    info::event_profiling::command_start) const {
    return impl_->profiling_command_start();
}

info::event_profiling::command_end::return_type event::get_profiling_info(
    info::event_profiling::command_end) const {
    return impl_->profiling_command_end();
}

CHARM_SYCL_END_NAMESPACE
