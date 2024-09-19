#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <charm/sycl/config.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

template <class T>
struct intrusive_ptr;

struct refcnt_base {
    refcnt_base(refcnt_base const&) = delete;
    refcnt_base(refcnt_base&&) = delete;
    refcnt_base& operator=(refcnt_base const&) = delete;
    refcnt_base& operator=(refcnt_base&&) = delete;
    virtual ~refcnt_base() = default;

protected:
    template <class T>
    friend struct intrusive_ptr;

    refcnt_base() noexcept : ref_(1) {}

    void retain() noexcept {
        ref_.fetch_add(1);
    }

    void release() {
        if (ref_.fetch_sub(1) == 1) {
            delete this;
        }
    }

private:
    template <class T>
    friend struct intrusive_ptr;

    bool try_unwrap() {
        uintptr_t one = 1;
        return ref_.compare_exchange_strong(one, 0);
    }

    std::atomic<uintptr_t> ref_;
};

template <class Derived>
struct enable_intrsuive_from_this {
protected:
    intrusive_ptr<Derived> intrusive_from_this()
        requires std::is_final_v<Derived>
    {
        return intrusive_ptr<Derived>(static_cast<Derived*>(this), true);
    }
};

template <class T>
struct intrusive_ptr {
    intrusive_ptr() noexcept : ptr_(nullptr) {}

    intrusive_ptr(intrusive_ptr const& other) noexcept : intrusive_ptr(other.ptr_, true) {}

    intrusive_ptr(intrusive_ptr&& other) noexcept : intrusive_ptr(other.release()) {}

    template <class Base>
    intrusive_ptr(intrusive_ptr<Base> const& ptr) noexcept
        requires std::is_base_of_v<T, Base>
        : intrusive_ptr(static_cast<T*>(ptr.ptr_), true) {}

    intrusive_ptr(std::unique_ptr<T>&& ptr) noexcept : intrusive_ptr(ptr.release()) {}

    template <class Base>
    intrusive_ptr(std::unique_ptr<Base>&& ptr) noexcept
        requires std::is_base_of_v<T, Base>
        : intrusive_ptr(static_cast<T*>(ptr.release())) {}

    intrusive_ptr& operator=(intrusive_ptr const& rhs) noexcept {
        intrusive_ptr(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept {
        intrusive_ptr(std::move(rhs)).swap(*this);
        return *this;
    }

    ~intrusive_ptr() {
        if (ptr_) {
            ptr_->release();
        }
    }

    std::unique_ptr<T> try_into_owned() && {
        if (ptr_->try_unwrap()) {
            return std::unique_ptr<T>{release()};
        }
        return nullptr;
    }

    explicit operator bool() const noexcept {
        return ptr_;
    }

    T* operator->() const noexcept {
        return ptr_;
    }

    T* get() const noexcept {
        return ptr_;
    }

    T& operator*() const noexcept {
        return *ptr_;
    }

    void swap(intrusive_ptr& other) noexcept {
        std::swap(ptr_, other.ptr_);
    }

    friend bool operator==(intrusive_ptr const& lhs, intrusive_ptr const& rhs) {
        return lhs.ptr_ == rhs.ptr_;
    }

    friend bool operator!=(intrusive_ptr const& lhs, intrusive_ptr const& rhs) {
        return lhs.ptr_ != rhs.ptr_;
    }

    template <class To>
    intrusive_ptr<To> static_pointer_cast() const {
        return intrusive_ptr<To>(static_cast<To*>(ptr_), true);
    }

private:
    template <class U>
    friend struct intrusive_ptr;

    template <class U>
    friend struct enable_intrsuive_from_this;

    explicit intrusive_ptr(T* ptr) : ptr_(ptr) {}

    explicit intrusive_ptr(T* ptr, bool) : ptr_(ptr) {
        if (ptr_) {
            ptr_->retain();
        }
    }

    T* release() {
        return std::exchange(ptr_, nullptr);
    }

    T* ptr_;
};

template <class T, class... Args>
intrusive_ptr<T> make_intrusive(Args&&... args) {
    auto p = std::make_unique<T>(std::forward<Args>(args)...);
    return intrusive_ptr<T>(std::move(p));
}

template <class To, class From>
intrusive_ptr<To> static_pointer_cast(intrusive_ptr<From> ptr) {
    return ptr.template static_pointer_cast<To>();
}

// Alternatives for std::vector and std::string

template <class T>
struct vec_view {
    vec_view(std::vector<T> const& vec) : data_(vec.data()), len_(vec.size()) {}

    T const* data() const {
        return data_;
    }

    size_t size() const {
        return len_;
    }

private:
    T const* data_ = nullptr;
    size_t len_ = 0;
};

template <class T>
struct vec {
    vec() : ptr_(nullptr), len_(0) {}

    explicit vec(size_t n);

    explicit vec(std::initializer_list<T> init) : vec(init.size()) {
        for (size_t i = 0; i < size(); i++) {
            ptr_[i] = init.begin()[i];
        }
    }

#if !defined(__clang_major__) || __clang_major__ >= 15
    vec(std::string const& str)
        requires std::is_same_v<T, char>
        : vec(str.data(), str.size()) {}
#else
    template <class U = T, class = std::enable_if_t<std::is_same_v<U, char>>>
    vec(std::string const& str) : vec(str.data(), str.size()) {}
#endif

    vec(T const* ptr, size_t len);

    vec(vec&& other) : ptr_(std::exchange(other.ptr_, nullptr)), len_(other.len_) {}

    vec(vec const& other) : vec(other.data(), other.size()) {}

    ~vec();

#if !defined(__clang_major__) || __clang_major__ >= 15
    operator std::string() const
        requires std::is_same_v<T, char>
#else
    template <class U = T, class = std::enable_if_t<std::is_same_v<U, char>>>
    operator std::string() const
#endif
    {
        return std::string(data(), size());
    }

    vec& operator=(vec&& rhs) {
        vec(std::move(rhs)).swap(*this);
        return *this;
    }

    vec& operator=(vec const& rhs) {
        vec(rhs).swap(*this);
        return *this;
    }

    T* data() {
        return ptr_;
    }

    T const* data() const {
        return ptr_;
    }

    size_t size() const {
        return len_;
    }

    T& operator[](size_t i) {
        return ptr_[i];
    }

    void push_back(T const& x);

    T* begin() {
        return ptr_;
    }

    T const* begin() const {
        return ptr_;
    }

    T* end() {
        return ptr_ ? ptr_ + len_ : nullptr;
        // return ptr_ + len_;
    }

    T const* end() const {
        return ptr_ ? ptr_ + len_ : nullptr;
        // return ptr_ + len_;
    }

    bool empty() const {
        return size() == 0;
    }

    void swap(vec& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(len_, other.len_);
    }

private:
    T* ptr_;
    size_t len_;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE

namespace std {

template <class T>
struct hash<CHARM_SYCL_NS::runtime::intrusive_ptr<T>> {
    size_t operator()(CHARM_SYCL_NS::runtime::intrusive_ptr<T> const& ptr) const {
        return std::hash<T*>()(ptr.get());
    }
};

}  // namespace std
