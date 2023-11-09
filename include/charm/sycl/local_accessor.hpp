#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <typename DataT, int Dimensions>
class local_accessor : detail::common_ref_ops<local_accessor<DataT, Dimensions>> {
    friend struct runtime::impl_access;

public:
    using value_type = DataT;
    using reference = value_type&;
    using const_reference = std::add_const_t<value_type>&;
    using iterator = value_type*;
    using const_iterator = std::add_const_t<value_type>*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;
    using size_type = size_t;
    using pointer_type = std::add_pointer_t<value_type>;

    template <int _Dim = Dimensions, class = std::enable_if_t<_Dim == 0>>
    local_accessor(handler& h, property_list const& = {});

    template <int _Dim = Dimensions, class = std::enable_if_t<(_Dim > 0)>>
    local_accessor(range<Dimensions> const& size, handler& h, property_list const& = {});

    size_type byte_size() const noexcept {
        return sizeof(DataT) * size();
    }

    size_type size() const noexcept;

    size_type max_size() const noexcept {
        return -1;
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    range<Dimensions> get_range() const;

    template <int _Dim = Dimensions, class = std::enable_if_t<_Dim == 0>>
    inline CHARM_SYCL_INLINE operator reference() const {
        return *this->get_pointer();
    }

    template <int _Dim = Dimensions, class = std::enable_if_t<_Dim == 0>>
    inline CHARM_SYCL_INLINE const local_accessor& operator=(const value_type& rhs) const {
        *this->get_pointer() = rhs;
        return *this;
    }

    template <int _Dim = Dimensions, class = std::enable_if_t<_Dim == 0>>
    inline CHARM_SYCL_INLINE const local_accessor& operator=(const value_type&& rhs) const {
        *this->get_pointer() = std::move(rhs);
        return *this;
    }

    template <int _Dim = Dimensions, class = std::enable_if_t<(_Dim > 0)>>
    inline CHARM_SYCL_INLINE reference operator[](item<Dimensions> const& index) const {
        if constexpr (Dimensions == 1) {
            return get_pointer()[index[0]];
        } else if constexpr (Dimensions == 2) {
            return get_pointer()[index[1] + index[0] * get_range()[1]];
        } else {
            return get_pointer()[index[2] + index[1] * get_range()[2] +
                                 index[0] * get_range()[2] * get_range()[1]];
        }
    }

    template <int _Dim = Dimensions, class = std::enable_if_t<(_Dim > 0)>>
    inline CHARM_SYCL_INLINE reference operator[](id<Dimensions> const& index) const {
        if constexpr (Dimensions == 1) {
            return get_pointer()[index[0]];
        } else if constexpr (Dimensions == 2) {
            return get_pointer()[index[1] + index[0] * get_range()[1]];
        } else {
            return get_pointer()[index[2] + index[1] * get_range()[2] +
                                 index[0] * get_range()[2] * get_range()[1]];
        }
    }

    pointer_type get_pointer() const noexcept;

    iterator begin() const noexcept {
        return get_pointer();
    }

    iterator end() const noexcept {
        return get_pointer() + byte_size();
    }

    const_iterator cbegin() const noexcept {
        return get_pointer();
    }

    const_iterator cend() const noexcept {
        return get_pointer() + byte_size();
    }

    reverse_iterator rbegin() const noexcept {
        std::make_reverse_iterator(end());
    }

    reverse_iterator rend() const noexcept {
        std::make_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const noexcept {
        std::make_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const noexcept {
        std::make_reverse_iterator(cbegin());
    }

private:
#ifdef __SYCL_DEVICE_ONLY__
    size_t off;
    size_t size0, size1, size2;
#else
    std::shared_ptr<runtime::local_accessor> impl_;
#endif
};

CHARM_SYCL_END_NAMESPACE
