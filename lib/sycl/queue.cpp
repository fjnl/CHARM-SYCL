#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

queue_impl::queue_impl(std::shared_ptr<context> ctx, std::shared_ptr<device> dev,
                       property_list const* props)
    : ctx_(ctx),
      dev_(dev),
      profiling_enabled_(
          props && props->get_property<sycl::property::queue::enable_profiling>() != nullptr) {
    events_.reserve(16);
}

sycl::backend queue_impl::get_backend() const noexcept {
    return sycl::backend::charm;
}

std::shared_ptr<device> queue_impl::get_device() const {
    return dev_;
}

std::shared_ptr<context> queue_impl::get_context() const {
    return ctx_;
}

void queue_impl::add(std::shared_ptr<runtime::event> const& ev) {
    events_.push_back(ev);
}

void queue_impl::wait() {
    if (!events_.empty()) {
        auto barrier = events_.front()->create_barrier();

        for (auto& ev : events_) {
            barrier->add(*ev);
        }

        barrier->wait();
    }
}

bool queue_impl::profiling_enabled() const {
    return profiling_enabled_;
}

}  // namespace runtime::impl

namespace runtime {

std::shared_ptr<queue> make_queue(std::shared_ptr<context> const& ctx,
                                  std::shared_ptr<device> const& dev, async_handler const&,
                                  property_list const* props) {
    return std::make_shared<impl::queue_impl>(ctx, dev, props);
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
