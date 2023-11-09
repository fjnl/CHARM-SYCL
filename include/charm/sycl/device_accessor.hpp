#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

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
            return range<Dimensions>(this->size2);
        } else if constexpr (Dimensions == 2) {
            return range<Dimensions>(this->size1, this->size2);
        } else {
            return range<Dimensions>(this->size0, this->size1, this->size2);
        }
#else
        return detail::shrink<Dimensions>(this->impl_->get_range());
#endif
    }

    inline CHARM_SYCL_INLINE id<Dimensions> get_offset() const {
#ifdef __SYCL_DEVICE_ONLY__
        if constexpr (Dimensions == 1) {
            return id<Dimensions>(this->offset2);
        } else if constexpr (Dimensions == 2) {
            return id<Dimensions>(this->offset1, this->offset2);
        } else {
            return id<Dimensions>(this->offset0, this->offset1, this->offset2);
        }
#else
        return detail::shrink<Dimensions>(this->impl_->get_offset());
#endif
    }

    inline CHARM_SYCL_INLINE pointer_type get_pointer() const noexcept {
#ifdef __SYCL_DEVICE_ONLY__
        return this->ptr;
#else
        return reinterpret_cast<pointer_type>(this->impl_->get_pointer());
#endif
    }

private:
    friend struct runtime::impl_access;

#ifdef __SYCL_DEVICE_ONLY__
    mutable pointer_type ptr;
    size_t size0, size1, size2;
    size_t offset0, offset1, offset2;
#else
    std::shared_ptr<runtime::accessor> impl_;
#endif
};

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
