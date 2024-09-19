#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

intrusive_ptr<runtime::event> make_event(std::unique_ptr<dep::event>&& ev) {
    return intrusive_ptr<runtime::event>(std::move(ev));
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
