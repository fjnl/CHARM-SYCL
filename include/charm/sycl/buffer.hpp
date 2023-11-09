#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <class T, int Dimensions, class AllocatorT>
struct buffer : detail::common_ref_ops<buffer<T, Dimensions, AllocatorT>> {
    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;
    using allocator_type = AllocatorT;

    inline buffer(const range<Dimensions>& bufferRange, const property_list& propList = {});

    inline buffer(const range<Dimensions>& bufferRange, AllocatorT allocator,
                  const property_list& propList = {});

    inline buffer(T* hostData, const range<Dimensions>& bufferRange,
                  const property_list& propList = {});

    inline buffer(T* hostData, const range<Dimensions>& bufferRange, AllocatorT allocator,
                  const property_list& propList = {});

    inline buffer(const T* hostData, const range<Dimensions>& bufferRange,
                  const property_list& propList = {});

    inline buffer(const T* hostData, const range<Dimensions>& bufferRange, AllocatorT allocator,
                  const property_list& propList = {});

    /* Available only if Container is a contiguous container:
         - std::data(container) and std::size(container) are well formed
         - return type of std::data(container) is convertible to T*
       and Dimensions == 1 */
    // template <class Container>
    // inline buffer(Container& container, AllocatorT allocator,
    //               const property_list& propList = {});

    /* Available only if Container is a contiguous container:
         - std::data(container) and std::size(container) are well formed
         - return type of std::data(container) is convertible to T*
       and Dimensions == 1 */
    // template <class Container>
    // inline buffer(Container& container, const property_list& propList = {});

    inline buffer(const std::shared_ptr<T>& hostData, const range<Dimensions>& bufferRange,
                  AllocatorT allocator, const property_list& propList = {});

    inline buffer(const std::shared_ptr<T>& hostData, const range<Dimensions>& bufferRange,
                  const property_list& propList = {});

    inline buffer(const std::shared_ptr<T[]>& hostData, const range<Dimensions>& bufferRange,
                  AllocatorT allocator, const property_list& propList = {});

    inline buffer(const std::shared_ptr<T[]>& hostData, const range<Dimensions>& bufferRange,
                  const property_list& propList = {});

    buffer(buffer const&) = default;

    buffer(buffer&&) = default;

    buffer& operator=(buffer const&) = default;

    buffer& operator=(buffer&&) = default;

    inline ~buffer();

    inline range<Dimensions> get_range() const;

    inline size_t byte_size() const noexcept;

    inline size_t size() const noexcept;

    // inline AllocatorT get_allocator() const;

    template <class Destination = std::nullptr_t>
    inline void set_final_data(Destination finalData = nullptr);

    inline void set_write_back(bool flag = true);

    inline bool is_sub_buffer() const {
        return false;
    }

    template <access_mode AccessMode = access_mode::read_write, target Target = target::device>
    inline accessor<T, Dimensions, AccessMode, Target> get_access(handler& commandGroupHandler);

    template <access_mode AccessMode>
    accessor<T, Dimensions, AccessMode, target::host_buffer> get_access();

    template <access_mode AccessMode = access_mode::read_write, target Target = target::device>
    accessor<T, Dimensions, AccessMode, Target> get_access(handler& commandGroupHandler,
                                                           range<Dimensions> accessRange,
                                                           id<Dimensions> accessOffset = {});

    template <access_mode AccessMode>
    accessor<T, Dimensions, AccessMode, target::host_buffer> get_access(
        range<Dimensions> accessRange, id<Dimensions> accessOffset = {});

    template <typename... Ts>
    auto get_access(Ts&&...);

    template <typename... Ts>
    auto get_host_access(Ts&&...);

private:
    friend struct runtime::impl_access;

    explicit buffer(std::shared_ptr<runtime::buffer> const& impl) : impl_(impl) {}

    std::shared_ptr<runtime::buffer> impl_;
};

template <class InputIterator, class AllocatorT>
buffer(InputIterator, InputIterator, AllocatorT, property_list const& = {})
    -> buffer<typename std::iterator_traits<InputIterator>::value_type, 1, AllocatorT>;

template <class InputIterator>
buffer(InputIterator, InputIterator, property_list const& = {})
    -> buffer<typename std::iterator_traits<InputIterator>::value_type, 1>;

template <class T, int Dimensions, class AllocatorT>
buffer(const T*, const range<Dimensions>&, AllocatorT, property_list const& = {})
    -> buffer<T, Dimensions, AllocatorT>;

template <class T, int Dimensions>
buffer(const T*, const range<Dimensions>&, property_list const& = {}) -> buffer<T, Dimensions>;

template <class Container, class AllocatorT>
buffer(Container&, AllocatorT, property_list const& = {})
    -> buffer<typename Container::value_type, 1, AllocatorT>;

template <class Container>
buffer(Container&, property_list const& = {}) -> buffer<typename Container::value_type, 1>;

CHARM_SYCL_END_NAMESPACE
