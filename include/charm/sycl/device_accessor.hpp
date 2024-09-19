#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

struct device_accessor_size {
    device_accessor_size() : size0(0), size1(0), size2(0), offset0(0), offset1(0), offset2(0) {}

    size_t size0, size1, size2;
    size_t offset0, offset1, offset2;
};

// Check the memory layout requirements
static_assert(std::is_trivially_copyable_v<device_accessor_size>,
              "Layout must match with the internal struct in the runtime library.");
static_assert(std::is_standard_layout_v<device_accessor_size>,
              "Layout must match with the internal struct in the runtime library.");
static_assert(sizeof(device_accessor_size) == sizeof(size_t) * 6,
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(device_accessor_size, size0) == 0,
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(device_accessor_size, size1) == 1 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(device_accessor_size, size2) == 2 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(device_accessor_size, offset0) == 3 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(device_accessor_size, offset1) == 4 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(device_accessor_size, offset2) == 5 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");

template <class DataT, int Dimensions, access_mode AccessMode>
struct device_accessor {
    using value_type = std::conditional_t<AccessMode == access_mode::read, const DataT, DataT>;
    using reference = std::add_lvalue_reference_t<value_type>;
    using const_reference = std::add_const_t<reference>;
    using pointer_type = std::add_pointer_t<value_type>;
    using iterator = pointer_type;
    using const_iterator = std::add_const_t<iterator>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;
    using size_type = size_t;

    device_accessor()
#ifdef __SYCL_DEVICE_ONLY__
        : ptr(nullptr),
          sz()
#else
        : impl_()
#endif
    {
    }

    /* Available only when: (Dimensions > 0) */
    template <typename AllocatorT, int _Dim = Dimensions, class = std::enable_if<(_Dim > 0)>>
    device_accessor(buffer<DataT, Dimensions, AllocatorT>& bufferRef,
                    handler& commandGroupHandlerRef, range<Dimensions> accessRange,
                    id<Dimensions> accessOffset, const property_list& propList = {});

    /* Available only when: (Dimensions > 0) */
    template <typename AllocatorT, int _Dim = Dimensions, class = std::enable_if<(_Dim > 0)>>
    device_accessor(buffer<DataT, Dimensions, AllocatorT>& bufferRef,
                    handler& commandGroupHandlerRef, const property_list& propList = {})
        : device_accessor(bufferRef, commandGroupHandlerRef, bufferRef.get_range(),
                          id<Dimensions>(), propList) {}

    /* Available only when: (Dimensions > 0) */
    template <typename AllocatorT, int _Dim = Dimensions, class = std::enable_if<(_Dim > 0)>>
    device_accessor(buffer<DataT, Dimensions, AllocatorT>& bufferRef,
                    const property_list& propList = {})
        : device_accessor(bufferRef, bufferRef.get_range(), id<Dimensions>(), propList) {}

    /* Available only when: (Dimensions > 0) */
    template <typename AllocatorT, int _Dim = Dimensions, class = std::enable_if<(_Dim > 0)>>
    device_accessor(buffer<DataT, Dimensions, AllocatorT>& bufferRef,
                    range<Dimensions> accessRange, const property_list& propList = {})
        : device_accessor(bufferRef, accessRange, id<Dimensions>(), propList) {}

    template <int _Dim = Dimensions>
    inline CHARM_SYCL_INLINE operator std::enable_if_t<_Dim == 0, reference>() const {
        return *this->get_pointer();
    }

    template <int _Dim = Dimensions>
    inline CHARM_SYCL_INLINE std::enable_if_t<_Dim == 0, const device_accessor&> operator=(
        const value_type& rhs) const {
        *this->get_pointer() = rhs;
        return *this;
    }

    template <int _Dim = Dimensions>
    inline CHARM_SYCL_INLINE std::enable_if_t<_Dim == 0, const device_accessor&> operator=(
        const value_type&& rhs) const {
        *this->get_pointer() = std::move(rhs);
        return *this;
    }

    template <int _Dim = Dimensions>
    inline CHARM_SYCL_INLINE std::enable_if_t<(_Dim > 0), reference> operator[](
        item<Dimensions> const& index) const {
        if constexpr (Dimensions == 1) {
            return get_pointer()[index[0]];
        } else if constexpr (Dimensions == 2) {
            return get_pointer()[index[1] + index[0] * get_range()[1]];
        } else {
            return get_pointer()[index[2] + index[1] * get_range()[2] +
                                 index[0] * get_range()[2] * get_range()[1]];
        }
    }

    template <int _Dim = Dimensions>
    inline CHARM_SYCL_INLINE std::enable_if_t<(_Dim > 0), reference> operator[](
        id<Dimensions> const& index) const {
        if constexpr (Dimensions == 1) {
            return get_pointer()[index[0]];
        } else if constexpr (Dimensions == 2) {
            return get_pointer()[index[1] + index[0] * get_range()[1]];
        } else {
            return get_pointer()[index[2] + index[1] * get_range()[2] +
                                 index[0] * get_range()[2] * get_range()[1]];
        }
    }

    inline CHARM_SYCL_INLINE bool is_placeholder() const {
        return false;
    }

    inline CHARM_SYCL_INLINE size_type max_size() const noexcept {
        return static_cast<size_type>(-1) / sizeof(DataT);
    }

    inline CHARM_SYCL_INLINE size_type byte_size() const noexcept {
        return size() * sizeof(DataT);
    }

    inline CHARM_SYCL_INLINE size_t get_size() const {
        return byte_size();
    }

    inline CHARM_SYCL_INLINE size_t get_count() const {
        return size();
    }

    inline CHARM_SYCL_INLINE bool empty() const noexcept {
        return size() == 0;
    }

    inline CHARM_SYCL_INLINE iterator begin() const noexcept {
        return this->get_pointer();
    }

    inline CHARM_SYCL_INLINE iterator end() const noexcept {
        return this->get_pointer() + this->size();
    }

    inline CHARM_SYCL_INLINE const_iterator cbegin() const noexcept {
        return this->get_pointer();
    }

    inline CHARM_SYCL_INLINE const_iterator cend() const noexcept {
        return this->get_pointer() + this->size();
    }

    inline CHARM_SYCL_INLINE reverse_iterator rbegin() const noexcept {
        return std::make_reverse_iterator(end());
    }

    inline CHARM_SYCL_INLINE reverse_iterator rend() const noexcept {
        return std::make_reverse_iterator(begin());
    }

    inline CHARM_SYCL_INLINE const_reverse_iterator crbegin() const noexcept {
        return std::make_reverse_iterator(cend());
    }

    inline CHARM_SYCL_INLINE const_reverse_iterator crend() const noexcept {
        return std::make_reverse_iterator(cbegin());
    }

    template <int _Dim = Dimensions>
    constexpr inline CHARM_SYCL_INLINE std::enable_if_t<(_Dim == 0), size_type> size()
        const noexcept {
        return 1;
    }

    template <int _Dim = Dimensions>
    inline CHARM_SYCL_INLINE std::enable_if_t<(_Dim > 0), size_type> size() const noexcept {
        return get_range().size();
    }

    inline CHARM_SYCL_INLINE range<Dimensions> get_range() const {
#ifdef __SYCL_DEVICE_ONLY__
        if constexpr (Dimensions == 1) {
            return range<Dimensions>(sz.size2);
        } else if constexpr (Dimensions == 2) {
            return range<Dimensions>(sz.size1, sz.size2);
        } else {
            return range<Dimensions>(sz.size0, sz.size1, sz.size2);
        }
#else
        if (this->impl_) {
            return detail::shrink<Dimensions>(this->impl_->get_range());
        }
        return detail::shrink<Dimensions>(range<3>(0, 0, 0));
#endif
    }

    inline CHARM_SYCL_INLINE id<Dimensions> get_offset() const {
#ifdef __SYCL_DEVICE_ONLY__
        if constexpr (Dimensions == 1) {
            return id<Dimensions>(sz.offset2);
        } else if constexpr (Dimensions == 2) {
            return id<Dimensions>(sz.offset1, sz.offset2);
        } else {
            return id<Dimensions>(sz.offset0, sz.offset1, sz.offset2);
        }
#else
        if (this->impl_) {
            return detail::shrink<Dimensions>(this->impl_->get_offset());
        }
        return detail::shrink<Dimensions>(id<3>(0, 0, 0));
#endif
    }

    inline CHARM_SYCL_INLINE pointer_type get_pointer() const noexcept {
#ifdef __SYCL_DEVICE_ONLY__
        return reinterpret_cast<pointer_type>(ptr);
#else
        return reinterpret_cast<pointer_type>(this->impl_->get_pointer());
#endif
    }

private:
    friend struct sycl::handler;
    friend struct runtime::impl_access;

#ifndef __SYCL_DEVICE_ONLY__
    void into_device() {
        auto const offset = this->impl_->get_offset();
        sz.offset0 = offset[0];
        sz.offset1 = offset[1];
        sz.offset2 = offset[2];

        auto const size = this->impl_->get_buffer()->get_range();
        sz.size0 = size[0];
        sz.size1 = size[1];
        sz.size2 = size[2];
    }
#endif

#ifdef __SYCL_DEVICE_ONLY__
    void* ptr;
#else
    runtime::accessor_ptr impl_;
#endif
    device_accessor_size sz;
};

static_assert(sizeof(accessor<int, 1, access_mode::read_write>) ==
              sizeof(void*) + sizeof(size_t) * 6);
#ifdef __SYCL_DEVICE_ONLY__
static_assert(std::is_standard_layout_v<accessor<int, 1, access_mode::read_write>>);
static_assert(std::is_trivially_copyable_v<accessor<int, 1, access_mode::read_write>>);
static_assert(std::is_trivially_destructible_v<accessor<int, 1, access_mode::read_write>>);
#endif

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
