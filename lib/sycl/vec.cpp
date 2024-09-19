#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

template <class T>
vec<T>::vec(size_t len) : ptr_(new T[len]), len_(len) {
    static_assert(std::is_nothrow_constructible_v<T>);

    for (size_t i = 0; i < size(); i++) {
        ptr_[i] = T();
    }
}

template <class T>
vec<T>::vec(T const* ptr, size_t len) : ptr_(new T[len]), len_(len) {
    static_assert(std::is_nothrow_copy_assignable_v<T>);

    for (size_t i = 0; i < size(); i++) {
        ptr_[i] = ptr[i];
    }
}

template <class T>
vec<T>::~vec() {
    if (auto p = std::exchange(ptr_, nullptr)) {
        delete[] p;
    }
}

template <class T>
void vec<T>::push_back(T const& x) {
    static_assert(std::is_nothrow_copy_assignable_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);

    auto tmp = vec(size() + 1);
    for (size_t i = 0; i < size(); i++) {
        tmp.ptr_[i] = std::move(ptr_[i]);
    }
    tmp[size()] = x;

    swap(tmp);
}

template struct vec<char>;
template struct vec<intrusive_ptr<platform>>;
template struct vec<intrusive_ptr<device>>;

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
