#pragma once

#include <charm/sycl/fwd.hpp>
#include <charm/sycl/range.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <int Dimensions>
struct nd_range {
    nd_range(range<Dimensions> globalSize, range<Dimensions> localSize)
        : global_size_(globalSize), local_size_(localSize) {}

    range<Dimensions> get_global_range() const {
        return global_size_;
    }

    range<Dimensions> get_local_range() const {
        return local_size_;
    }

    range<Dimensions> get_group_range() const {
        return global_size_ / local_size_;
    }

    friend bool operator==(nd_range const& lhs, nd_range const& rhs) {
        return lhs.global_size_ == rhs.global_size_ && lhs.local_size_ == rhs.local_size_;
    }

    friend bool operator!=(nd_range const& lhs, nd_range const& rhs) {
        return !(lhs == rhs);
    }

private:
    range<Dimensions> global_size_;
    range<Dimensions> local_size_;
};

CHARM_SYCL_END_NAMESPACE
