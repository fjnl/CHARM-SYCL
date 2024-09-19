#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(const range<Dimensions>& bufferRange,
                                          const property_list& propList)
    : buffer(bufferRange, AllocatorT(), propList) {}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(const range<Dimensions>& bufferRange,
                                          AllocatorT allocator, const property_list& propList)
    : buffer(static_cast<T*>(nullptr), bufferRange, allocator, propList) {}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(T* hostData, const range<Dimensions>& bufferRange,
                                          const property_list& propList)
    : buffer(hostData, bufferRange, AllocatorT(), propList) {}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(T* hostData, const range<Dimensions>& bufferRange,
                                          [[maybe_unused]] AllocatorT allocator,
                                          const property_list&)
    : impl_(runtime::make_buffer(hostData, sizeof(T), detail::extend(bufferRange))) {
    if (hostData) {
        set_final_data(hostData);
    }
}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(const T* hostData,
                                          const range<Dimensions>& bufferRange,
                                          const property_list& propList)
    : buffer(hostData, bufferRange, AllocatorT(), propList) {}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(const T* hostData,
                                          const range<Dimensions>& bufferRange,
                                          [[maybe_unused]] AllocatorT allocator,
                                          const property_list&)
    : impl_(runtime::make_buffer(const_cast<T*>(hostData), sizeof(T),
                                 detail::extend(bufferRange))) {}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(const std::shared_ptr<T>& hostData,
                                          const range<Dimensions>& bufferRange,
                                          [[maybe_unused]] AllocatorT allocator,
                                          const property_list&)
    : impl_(runtime::make_buffer(hostData.get(), sizeof(T), detail::extend(bufferRange))),
      sp_(hostData) {
    set_final_data(hostData.get());
}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(const std::shared_ptr<T>& hostData,
                                          const range<Dimensions>& bufferRange,
                                          const property_list& propList)
    : buffer(hostData, bufferRange, AllocatorT(), propList) {}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(const std::shared_ptr<T[]>& hostData,
                                          const range<Dimensions>& bufferRange,
                                          [[maybe_unused]] AllocatorT allocator,
                                          const property_list&)
    : impl_(runtime::make_buffer(hostData.get(), sizeof(T), detail::extend(bufferRange))),
      sp_(hostData) {
    set_final_data(hostData.get());
}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::buffer(const std::shared_ptr<T[]>& hostData,
                                          const range<Dimensions>& bufferRange,
                                          const property_list& propList)
    : buffer(hostData, bufferRange, AllocatorT(), propList) {}

template <class T, int Dimensions, class AllocatorT>
buffer<T, Dimensions, AllocatorT>::~buffer() {
    if (impl_) {
        if (auto owned = std::move(impl_).try_into_owned()) {
            owned->write_back();
        }
    }
}

template <class T, int Dimensions, class AllocatorT>
range<Dimensions> buffer<T, Dimensions, AllocatorT>::get_range() const {
    return detail::shrink<Dimensions>(impl_->get_range());
}

template <class T, int Dimensions, class AllocatorT>
size_t buffer<T, Dimensions, AllocatorT>::byte_size() const noexcept {
    return size() * sizeof(T);
}

template <class T, int Dimensions, class AllocatorT>
size_t buffer<T, Dimensions, AllocatorT>::size() const noexcept {
    return get_range().size();
}

// template <class T, int Dimensions, class AllocatorT>
// AllocatorT buffer<T, Dimensions, AllocatorT>::get_allocator() const {
//     return impl_->get_allocator();
// }

template <class T, int Dimensions, class AllocatorT>
template <class Destination>
void buffer<T, Dimensions, AllocatorT>::set_final_data(Destination finalData) {
    if constexpr (std::is_null_pointer_v<Destination>) {
        impl_->set_final_pointer(nullptr);
        impl_->set_write_back(false);
    } else if constexpr (std::is_pointer_v<Destination>) {
        impl_->set_final_pointer(finalData);
        impl_->set_write_back(true);
    } else {
        static_assert(not_supported<Destination>,
                      "non-pointer set_final_data is not supported.");
    }
}

//     using base_type = runtime::final_data_handler_base<T const*>;

// auto h = std::unique_ptr<base_type>(
//     new runtime::final_data_handler<T const*, Destination>(finalData));
// impl_->set_final_data(std::move(h));
// }

template <class T, int Dimensions, class AllocatorT>
void buffer<T, Dimensions, AllocatorT>::set_write_back(bool flag) {
    impl_->set_write_back(flag);
}

template <class T, int Dimensions, class AllocatorT>
template <access_mode AccessMode, target Target>
inline accessor<T, Dimensions, AccessMode, Target>
buffer<T, Dimensions, AllocatorT>::get_access(handler& commandGroupHandler) {
    static_assert(Target == target::device);
    return accessor<T, Dimensions, AccessMode, Target>(*this, commandGroupHandler);
}

template <class T, int Dimensions, class AllocatorT>
template <access_mode AccessMode>
inline accessor<T, Dimensions, AccessMode, target::host_buffer>
buffer<T, Dimensions, AllocatorT>::get_access() {
    return accessor<T, Dimensions, AccessMode, target::host_buffer>(*this);
}

template <class T, int Dimensions, class AllocatorT>
template <access_mode AccessMode, target Target>
accessor<T, Dimensions, AccessMode, Target> buffer<T, Dimensions, AllocatorT>::get_access(
    handler& commandGroupHandler, range<Dimensions> accessRange, id<Dimensions> accessOffset) {
    return accessor<T, Dimensions, AccessMode, Target>(*this, commandGroupHandler, accessRange,
                                                       accessOffset);
}

template <class T, int Dimensions, class AllocatorT>
template <access_mode AccessMode>
accessor<T, Dimensions, AccessMode, target::host_buffer>
buffer<T, Dimensions, AllocatorT>::get_access(range<Dimensions> accessRange,
                                              id<Dimensions> accessOffset) {
    return accessor<T, Dimensions, AccessMode, target::host_buffer>(*this, accessRange,
                                                                    accessOffset);
}

template <class T, int Dimensions, class AllocatorT>
template <typename... Ts>
auto buffer<T, Dimensions, AllocatorT>::get_access(Ts&&... args) {
    return accessor(*this, std::forward<Ts>(args)...);
}

template <class T, int Dimensions, class AllocatorT>
template <typename... Ts>
auto buffer<T, Dimensions, AllocatorT>::get_host_access(Ts&&... args) {
    return accessor(*this, std::forward<Ts>(args)...);
}

CHARM_SYCL_END_NAMESPACE
