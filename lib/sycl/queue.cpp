#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

queue_impl::queue_impl(intrusive_ptr<context> ctx, intrusive_ptr<device> dev,
                       sycl::property::queue::enable_profiling const* enable_profiling)
    : ctx_(ctx), dev_(dev), profiling_enabled_(enable_profiling != nullptr) {
    events_.reserve(16);
}

sycl::backend queue_impl::get_backend() const noexcept {
    return sycl::backend::charm;
}

intrusive_ptr<device> queue_impl::get_device() const {
    return dev_;
}

intrusive_ptr<context> queue_impl::get_context() const {
    return ctx_;
}

void queue_impl::add(intrusive_ptr<runtime::event> const& ev) {
    events_.push_back(ev);
}

void queue_impl::wait() {
    if (!events_.empty()) {
        auto events = std::move(events_);
        auto barrier = events.front()->create_barrier();

        for (auto& ev : events) {
            barrier->add(*ev);
        }

        barrier->wait();

        events.front()->release_barrier(barrier);
    }
}

bool queue_impl::profiling_enabled() const {
    return profiling_enabled_;
}

}  // namespace runtime::impl

namespace runtime {

intrusive_ptr<queue> make_queue(
    intrusive_ptr<context> const& ctx, intrusive_ptr<device> const& dev,
    sycl::property::queue::enable_profiling const* enable_profiling) {
    return make_intrusive<impl::queue_impl>(ctx, dev, enable_profiling);
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
