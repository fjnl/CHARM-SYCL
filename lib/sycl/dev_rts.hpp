#pragma once

#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <BS_thread_pool.hpp>
#include "kreg.hpp"
#include "rts.hpp"

namespace dev_rts {

namespace rts = CHARM_SYCL_NS::rts;

extern std::unique_ptr<BS::thread_pool> q_task;
extern std::chrono::high_resolution_clock::time_point t0;

inline void init_time_point() {
    t0 = std::chrono::high_resolution_clock::now();
}

template <class Derived>
struct event_node : std::enable_shared_from_this<Derived> {
    using event_ptr = std::shared_ptr<Derived>;
    using weak_event_ptr = std::weak_ptr<Derived>;

    virtual ~event_node() = default;

    event_node(event_node const&) = delete;

    event_node(event_node&&) = delete;

    event_node& operator=(event_node const&) = delete;

    event_node& operator=(event_node&&) = delete;

    using fn_t = std::function<void(event_ptr)>;

    bool happens_before(event_ptr next) {
        std::unique_lock lk(mutex_);

        if (!done_) {
            nexts_.push_back(next);

            std::unique_lock lk_n(next->mutex_);
            next->n_wait_ += 1;
        }

        return !done_;
    }

    void enable_profiling() {
        t_enable = true;
    }

    uint64_t get_t_submit() const {
        return t_submit;
    }

    uint64_t get_t_start() const {
        return t_start;
    }

    uint64_t get_t_end() const {
        return t_end;
    }

    void finalize() {
        if (t_enable) [[unlikely]] {
            auto const t = std::chrono::high_resolution_clock::now();
            t_submit = std::chrono::duration_cast<std::chrono::nanoseconds>(t - t0).count();
        }

        notify();
    }

    void notify() {
        std::unique_lock lk(mutex_);

        n_wake_ += 1;
        check_to_run();
    }

    void complete() {
        std::unique_lock lk(mutex_);
        complete_no_lock();
    }

    void set_fn(std::function<void(event_ptr)> const& f) {
        fn_ = f;
    }

    static auto create() {
        return std::shared_ptr<Derived>(new Derived);
    }

protected:
    event_node() = default;

private:
    void complete_no_lock() {
        done_ = true;

        if (t_enable) [[unlikely]] {
            auto const t = std::chrono::high_resolution_clock::now();
            t_end = std::chrono::duration_cast<std::chrono::nanoseconds>(t - t0).count();
        }

        for (auto n : nexts_) {
            n->notify();
        }
    }

    void check_to_run() {
        if (!run_ && n_wake_ == n_wait_) {
            run_ = true;

            if (fn_) {
                q_task->detach_task([ev = this->shared_from_this()] {
                    if (ev->t_enable) [[unlikely]] {
                        auto const t = std::chrono::high_resolution_clock::now();
                        ev->t_start =
                            std::chrono::duration_cast<std::chrono::nanoseconds>(t - t0)
                                .count();
                    }

                    ev->fn_(ev);
                });
            } else {
                if (t_enable) [[unlikely]] {
                    auto const t = std::chrono::high_resolution_clock::now();
                    t_start =
                        std::chrono::duration_cast<std::chrono::nanoseconds>(t - t0).count();
                }

                complete_no_lock();
            }
        }
    }

    mutable std::mutex mutex_;
    uint16_t n_wake_ = 0;
    uint16_t n_wait_ = 1;
    bool done_ = false;
    bool run_ = false;
    bool t_enable = false;
    uint64_t t_submit = 0;
    uint64_t t_start = 0;
    uint64_t t_end = 0;
    std::vector<event_ptr> nexts_;
    std::function<void(event_ptr)> fn_;
};

struct memory_domain_impl : rts::memory_domain {
    rts::dom_id id() const override {
        return 1;
    }
};

template <class EventNode>
struct event_base;

template <class EventNode>
struct event_barrier_impl : rts::event_barrier {
    event_barrier_impl();

    void add(sycl::runtime::event& event) override;

    void add(event_base<EventNode>& ev);

    void wait() override;

private:
    void finalize_and_wait();

    std::shared_ptr<EventNode> sync_;
    bool need_wait_ = false;
};

template <class EventNode>
struct event_base : rts::event {
    explicit event_base(std::shared_ptr<EventNode> const& ev) : ev_(ev) {}

    explicit event_base(std::shared_ptr<EventNode>&& ev) : ev_(std::move(ev)) {}

    sycl::runtime::event_barrier* create_barrier() override {
        return new event_barrier_impl<EventNode>();
    }

    void release_barrier(sycl::runtime::event_barrier* ptr) override {
        delete ptr;
    }

    uint64_t profiling_command_submit() override {
        event_barrier_impl<EventNode> barrier;
        barrier.add(*this);
        barrier.wait();
        return ev_->get_t_submit();
    }

    uint64_t profiling_command_start() override {
        event_barrier_impl<EventNode> barrier;
        barrier.add(*this);
        barrier.wait();
        return ev_->get_t_start();
    }

    uint64_t profiling_command_end() override {
        event_barrier_impl<EventNode> barrier;
        barrier.add(*this);
        barrier.wait();
        return ev_->get_t_end();
    }

protected:
    friend struct event_barrier_impl<EventNode>;

    std::shared_ptr<EventNode> ev_;
};

template <class EventNode>
event_barrier_impl<EventNode>::event_barrier_impl() : sync_(EventNode::create()) {}

template <class EventNode>
void event_barrier_impl<EventNode>::add(sycl::runtime::event& event) {
    add(static_cast<event_base<EventNode>&>(event));
}

template <class EventNode>
void event_barrier_impl<EventNode>::add(event_base<EventNode>& ev) {
    auto impl = ev.ev_;

    if (impl->happens_before(sync_)) {
        need_wait_ = true;
    }
}

template <class EventNode>
void event_barrier_impl<EventNode>::wait() {
    if (need_wait_) {
        finalize_and_wait();
    }
}

template <class EventNode>
void event_barrier_impl<EventNode>::finalize_and_wait() {
    std::condition_variable cv;
    std::mutex m;
    bool ready = false;

    sync_->set_fn([&](std::shared_ptr<EventNode> const& ev) {
        std::unique_lock lk(m);

        ready = true;
        cv.notify_all();
        lk.unlock();

        ev->complete();
    });

    std::unique_lock lk(m);

    sync_->finalize();

    cv.wait(lk, [&] {
        return ready;
    });
}

extern memory_domain_impl g_dom;

struct device_base : rts::device {
    bool is_cpu() const override {
        return true;
    }

    bool is_gpu() const override {
        return false;
    }

    bool is_accelerator() const override {
        return false;
    }

    bool is_fpga() const override {
        return false;
    }

    bool is_custom() const override {
        return false;
    }

    bool is_host() const override {
        return false;
    }

    rts::memory_domain& get_memory_domain() const override {
        return g_dom;
    }

    std::string info_vendor() const override {
        return "Dev-RTS";
    }

    std::string info_driver_version() const override {
        return "none";
    }
};

template <class DeviceImpl>
struct platform_base : rts::platform {
    std::vector<std::shared_ptr<rts::device>> get_devices() const override {
        std::vector<std::shared_ptr<rts::device>> devs;

        devs.push_back(std::make_shared<DeviceImpl>());

        return devs;
    };

    std::string info_vendor() const override {
        return "Dev-RTS";
    }

    std::string info_version() const override {
        return "dev";
    }
};

template <class Platform, class Buffer>
struct subsystem_base : rts::subsystem {
    std::vector<std::shared_ptr<rts::platform>> get_platforms() const override {
        std::vector<std::shared_ptr<rts::platform>> plts;
        plts.push_back(std::make_shared<Platform>());
        return plts;
    }

    std::unique_ptr<rts::buffer> new_buffer(void* h_ptr, size_t element_size,
                                            rts::range const& size) override {
        return std::make_unique<Buffer>(h_ptr, element_size, size);
    }

    rts::memory_domain& get_host_memory_domain() override {
        return host_;
    }

private:
    rts::host_memory_domain host_;
};

struct buffer_base : rts::buffer {
    explicit buffer_base(void* h_ptr, size_t element_size, rts::range const& size)
        : h_ptr_(h_ptr), element_size_(element_size), size_(size) {}

    ~buffer_base() override {}

    void* get_host_pointer() override {
        return h_ptr_;
    }

    rts::range const& get_size() const override {
        return size_;
    }

    rts::range offset() {
        return rts::range(0, 0, 0);
    }

    size_t byte_size() const {
        return element_size_ * size_.size[0] * size_.size[1] * size_.size[2];
    }

    size_t elem_size() const {
        return element_size_;
    }

private:
    void* h_ptr_;
    size_t element_size_;
    rts::range size_;
};

struct task_parameter_storage {
    static constexpr size_t MAX_N_ARGS = 64;
    static constexpr size_t ARG_BUF_SIZE = 4096;

    task_parameter_storage() {
        clear();
    }

    void clear() {
        args_.fill(nullptr);
        arg_next_ = arg_data_.data();
        arg_idx_ = 0;
    }

    void* next_param_ptr(size_t size, size_t align) {
        if (arg_idx_ >= MAX_N_ARGS) {
            assert(arg_idx_ < MAX_N_ARGS);
            return nullptr;
        }
        assert(arg_next_ < arg_data_.data() + arg_data_.size());

        auto addr = reinterpret_cast<uintptr_t>(arg_next_);
        auto const mask = static_cast<uintptr_t>(align) - 1;

        if (addr & mask) {
            addr = (addr & ~mask) + align;
        }
        auto const ptr = reinterpret_cast<std::byte*>(addr);

        args_.at(arg_idx_) = ptr;
        arg_idx_++;
        arg_next_ = ptr + size;

        return ptr;
    }

    void* next_param_ptr(size_t size) {
        return next_param_ptr(size, alignof(std::max_align_t));
    }

    template <class T>
    T* next_param_ptr() {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);
        return reinterpret_cast<T*>(next_param_ptr(sizeof(T), alignof(T)));
    }

    void add_param_val(void* val) {
        args_.at(arg_idx_) = val;
        arg_idx_++;
    }

    void** data() {
        return args_.data();
    }

protected:
    std::array<void*, MAX_N_ARGS> args_;
    void* arg_next_;
    unsigned int arg_idx_;
    alignas(std::max_align_t) std::array<char, ARG_BUF_SIZE> arg_data_;
};

inline void* advance_ptr(void* ptr, size_t off_byte) {
    return reinterpret_cast<std::byte*>(ptr) + off_byte;
}

inline void const* advance_ptr(void const* ptr, size_t off_byte) {
    return reinterpret_cast<std::byte const*>(ptr) + off_byte;
}

}  // namespace dev_rts
