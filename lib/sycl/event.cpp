#include <charm/sycl.hpp>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

std::shared_ptr<runtime::event> make_event(std::shared_ptr<dep::event> const& ev) {
    return ev;
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
