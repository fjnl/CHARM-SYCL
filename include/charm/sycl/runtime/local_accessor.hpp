#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime {

struct local_accessor {
    virtual ~local_accessor() = default;

    size_t size() const {
        return range_[0] * range_[1] * range_[2];
    }

    range<3> const& get_range() const {
        return range_;
    }

    void* get_pointer() const noexcept {
        return nullptr;
    }

protected:
    explicit local_accessor(range<3> const& range) : range_(range) {}

    range<3> range_;
};

}  // namespace runtime
CHARM_SYCL_END_NAMESPACE
