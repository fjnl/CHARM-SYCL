#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

buffer_impl::buffer_impl(void* init_ptr, size_t elemsize, sycl::range<3> const& rng)
    : elemsize_(elemsize),
      range_(rng),
      write_back_(false),
      write_back_ptr_(nullptr),
      dep_(impl::global_state::get_depmgr()->new_buffer(init_ptr, elemsize,
                                                        impl::convert(rng))) {}

void buffer_impl::write_back() {
    write_back(dep::memory_access::read_write);
}

void buffer_impl::write_back(dep::memory_access acc) {
    if (!write_back_) {
        return;
    }
    if (!write_back_ptr_) {
        return;
    }
    do_writeback(acc);
}

sycl::range<3> buffer_impl::get_range() const {
    return range_;
}

size_t buffer_impl::byte_size() const {
    return elemsize_ * range_.size();
}

void buffer_impl::set_write_back(bool on) {
    write_back_ = on;
}

void buffer_impl::set_final_pointer(void* ptr) {
    write_back_ptr_ = ptr;
}

void buffer_impl::do_writeback(dep::memory_access acc) {
    auto task = global_state::get_depmgr()->new_task();
    task->use_host();
    task->set_host_fn();
    task->begin_params();
    try {
        task->set_buffer_param(*dep_, acc, dep::id(), 0);
    } catch (...) {
        task->end_params();
        throw;
    }
    task->end_params();

    auto ev = task->submit();
    auto barrier = ev->create_barrier();
    barrier->add(*ev);
    barrier->wait();
    ev->release_barrier(barrier);
}

void* buffer_impl::get_pointer() {
    return dep_->get_pointer();
}

void* buffer_impl::get_host_pointer() {
    return dep_->get_host_pointer();
}

std::shared_ptr<dep::buffer> buffer_impl::to_lower() {
    return dep_;
}

size_t buffer_impl::elem_size() const {
    return elemsize_;
}

intrusive_ptr<buffer_impl> make_buffer(void* init_ptr, size_t elemsize, range<3> const& rng) {
    return make_intrusive<buffer_impl>(init_ptr, elemsize, rng);
}

}  // namespace runtime::impl

namespace runtime {

intrusive_ptr<buffer> make_buffer(void* init_ptr, size_t elemsize, range<3> const& rng) {
    return impl::make_buffer(init_ptr, elemsize, rng);
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
