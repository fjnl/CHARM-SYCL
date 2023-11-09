#pragma once

#include <memory>
#include <stdlib.h>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

template <class T>
struct allocator {
    using size_type = size_t;
    using pointer = std::add_pointer_t<T>;
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    constexpr allocator() noexcept = default;

    constexpr allocator(const allocator&) noexcept = default;

    template <class U>
    constexpr allocator(const allocator<U>&) noexcept {}

    ~allocator() = default;

    constexpr allocator& operator=(const allocator&) = default;

    [[nodiscard]] constexpr auto allocate(size_type n) {
        pointer ptr = nullptr;
        posix_memalign((void**)&ptr, 4096, sizeof(T) * n);
        return ptr;
    }

    constexpr void deallocate(pointer p, size_type) {
        free(p);
    }

    inline friend bool operator==(allocator const&, allocator const&) {
        return true;
    }

    inline friend bool operator!=(allocator const&, allocator const&) {
        return false;
    }
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
