#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime::impl {

void fiber_init();
void exec_with_fibers(size_t group_range1, size_t group_range2, size_t group_range3,
                      size_t local_range1, size_t local_range2, size_t local_range3,
                      size_t lmem_byte, std::function<void(void**)> const& fn, void** args);

}  // namespace runtime::impl
CHARM_SYCL_END_NAMESPACE
