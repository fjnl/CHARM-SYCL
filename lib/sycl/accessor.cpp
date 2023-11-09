#include <charm/sycl.hpp>
#include "rt.hpp"
#include "rts.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

accessor_impl::accessor_impl(std::shared_ptr<handler_impl> const& handler,
                             std::shared_ptr<buffer_impl> const& buf, range<3> range,
                             id<3> offset, access_mode mode, property_list const*)
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

std::shared_ptr<runtime::buffer> accessor_impl::get_buffer() {
    return buffer_;
}

std::shared_ptr<buffer_impl> accessor_impl::get() {
    return buffer_;
}

access_mode accessor_impl::get_access_mode() const {
    return mode_;
}

std::shared_ptr<accessor_impl> make_accessor(std::shared_ptr<runtime::handler> const& handler,
                                             std::shared_ptr<runtime::buffer> const& buf,
                                             range<3> range, id<3> offset, access_mode mode,
                                             property_list const* props) {
    return std::make_shared<accessor_impl>(std::dynamic_pointer_cast<handler_impl>(handler),
                                           std::dynamic_pointer_cast<buffer_impl>(buf), range,
                                           offset, mode, props);
}

host_accessor_impl::host_accessor_impl(std::shared_ptr<buffer_impl> const& buf, range<3> range,
                                       id<3> offset, access_mode mode, property_list const*)
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

std::shared_ptr<runtime::buffer> host_accessor_impl::get_buffer() {
    return buffer_;
}

std::shared_ptr<buffer_impl> host_accessor_impl::get() {
    return buffer_;
}

access_mode host_accessor_impl::get_access_mode() const {
    return mode_;
}

std::shared_ptr<host_accessor_impl> make_host_accessor(
    std::shared_ptr<runtime::buffer> const& buf, range<3> range, id<3> offset, access_mode mode,
    property_list const* props) {
    return std::make_shared<host_accessor_impl>(std::dynamic_pointer_cast<buffer_impl>(buf),
                                                range, offset, mode, props);
}

}  // namespace runtime::impl

namespace runtime {

std::shared_ptr<accessor> make_accessor(std::shared_ptr<handler> const& handler,
                                        std::shared_ptr<buffer> const& buf, range<3> range,
                                        id<3> offset, access_mode mode,
                                        property_list const* props) {
    return impl::make_accessor(handler, buf, range, offset, mode, props);
}

std::shared_ptr<host_accessor> make_host_accessor(std::shared_ptr<buffer> const& buf,
                                                  range<3> range, id<3> offset,
                                                  access_mode mode,
                                                  property_list const* props) {
    return impl::make_host_accessor(buf, range, offset, mode, props);
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
