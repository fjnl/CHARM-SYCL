#include <charm/sycl.hpp>
#include "rt.hpp"
#include "rts.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

accessor_impl::accessor_impl(intrusive_ptr<handler_impl> const& handler,
                             intrusive_ptr<buffer_impl> const& buf, range<3> range,
                             id<3> offset, access_mode mode)
    : range_(range), offset_(offset), handler_(handler), buffer_(buf), mode_(mode) {}

range<3> accessor_impl::get_range() const {
    return range_;
}

id<3> accessor_impl::get_offset() const {
    return offset_;
}

void* accessor_impl::get_pointer() {
    return buffer_->get_pointer();
}

runtime::buffer_ptr accessor_impl::get_buffer() {
    return buffer_;
}

intrusive_ptr<buffer_impl> accessor_impl::get() {
    return buffer_;
}

access_mode accessor_impl::get_access_mode() const {
    return mode_;
}

intrusive_ptr<accessor_impl> make_accessor(intrusive_ptr<runtime::handler> const& handler,
                                           intrusive_ptr<runtime::buffer> const& buf,
                                           range<3> range, id<3> offset, access_mode mode) {
    return make_intrusive<accessor_impl>(static_pointer_cast<handler_impl>(handler),
                                         static_pointer_cast<buffer_impl>(buf), range, offset,
                                         mode);
}

host_accessor_impl::host_accessor_impl(intrusive_ptr<buffer_impl> const& buf, range<3> range,
                                       id<3> offset, access_mode mode)
    : range_(range), offset_(offset), buffer_(buf), mode_(mode) {
    switch (mode) {
        case access_mode::read:
            buf->do_writeback(dep::memory_access::read_only);
            break;
        case access_mode::write:
        case access_mode::read_write:
        case access_mode::discard_write:
        case access_mode::discard_read_write:
            buf->do_writeback(dep::memory_access::read_write);
            break;
        case access_mode::atomic:
            fprintf(stderr, "Error: std::runtime not supported\n");
            std::abort();
            break;
    }
}

range<3> host_accessor_impl::get_range() const {
    return range_;
}

id<3> host_accessor_impl::get_offset() const {
    return offset_;
}

void* host_accessor_impl::get_pointer() {
    return buffer_->get_host_pointer();
}

runtime::buffer_ptr host_accessor_impl::get_buffer() {
    return buffer_;
}

intrusive_ptr<buffer_impl> host_accessor_impl::get() {
    return buffer_;
}

access_mode host_accessor_impl::get_access_mode() const {
    return mode_;
}

intrusive_ptr<host_accessor_impl> make_host_accessor(runtime::buffer_ptr const& buf,
                                                     range<3> range, id<3> offset,
                                                     access_mode mode) {
    return make_intrusive<host_accessor_impl>(static_pointer_cast<buffer_impl>(buf), range,
                                              offset, mode);
}

}  // namespace runtime::impl

namespace runtime {

accessor_ptr make_accessor(intrusive_ptr<handler> const& handler,
                           intrusive_ptr<buffer> const& buf, range<3> range, id<3> offset,
                           access_mode mode) {
    return impl::make_accessor(handler, buf, range, offset, mode);
}

host_accessor_ptr make_host_accessor(intrusive_ptr<buffer> const& buf, range<3> range,
                                     id<3> offset, access_mode mode) {
    return impl::make_host_accessor(buf, range, offset, mode);
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
