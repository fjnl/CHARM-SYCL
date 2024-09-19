#include "coarse_task.hpp"
#include <barrier>
#include <cstdio>
#include "../format.hpp"
#include "../logging.hpp"

#define NON_NULL(expr)                                                                \
    ({                                                                                \
        auto ptr = (expr);                                                            \
        if (ptr == nullptr) {                                                         \
            fprintf(stderr, "Error: `%s` is null at %s:%d in %s.\n", #expr, __FILE__, \
                    __LINE__, __PRETTY_FUNCTION__);                                   \
        }                                                                             \
        ptr;                                                                          \
    })

namespace {
LOGGING_DEFINE_SCOPE(task)

#ifdef CHARM_SYCL_ENABLE_LOGGING
struct logging_initializer {
    logging_initializer() {
        init_logging();
    }
};
[[maybe_unused]] inline logging_initializer _init;
#endif

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE
namespace dev_rts {

struct event final : rts::event {
    explicit event(task_weak_ptr const& wk) : weak_(wk) {}

    sycl::runtime::event_barrier* create_barrier() override;

    void release_barrier(sycl::runtime::event_barrier* ptr) override;

    uint64_t profiling_command_submit() override {
        return -1;
    }

    uint64_t profiling_command_start() override {
        return -1;
    }

    uint64_t profiling_command_end() override {
        return -1;
    }

    task_ptr lock() const;

private:
    task_weak_ptr weak_;
};

struct event_barrier final : rts::event_barrier {
    event_barrier();

    void add(sycl::runtime::event& event) override;

    void wait() override;

private:
    std::barrier<> bar_;
    task_ptr sync_;
    bool empty_ = true;
};

struct sync_op : op_base {
    explicit sync_op(std::barrier<>& bar) : bar_(bar) {}

    void call(task_ptr const& task) override {
        bar_.arrive_and_drop();
        task->complete();
    }

    std::barrier<>& bar_;
};

sycl::runtime::event_barrier* event::create_barrier() {
    return new event_barrier();
}

void event::release_barrier(sycl::runtime::event_barrier* ptr) {
    delete ptr;
}

task_ptr event::lock() const {
    return weak_.lock();
}

event_barrier::event_barrier() : bar_(2), sync_(make_task_from_op(sync_op(bar_))) {}

void event_barrier::add(sycl::runtime::event& event) {
    DEBUG_FMT("this={} event_barrier::{}()", format::ptr(this), __func__);

    if (auto t = static_cast<struct event&>(event).lock()) {
        DEBUG_FMT("\t{} runs after {}", format::ptr(sync_.get()), format::ptr(t.get()));
        sync_->runs_after(t);
        empty_ = false;
    }
}

void event_barrier::wait() {
    if (!empty_) {
        sync_->finalize();
        bar_.arrive_and_wait();
        DEBUG_FMT("this={} event_barrier::{}() sync", format::ptr(this), __func__);
    }
}

coarse_task::coarse_task() : k_() {
    depends_.reserve(8);
}

void coarse_task::commit() {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    for (auto& d : depends_) {
        if (!kernel_) {
            kernel_ = make_op_task(make_kernel_op());
            DEBUG_FMT("\tkernel_={}", format::ptr(kernel_.get()));
        }

        DEBUG_FMT("\ttask {} runs after {}", format::ptr(kernel_.get()), format::ptr(d.get()));
        kernel_->runs_after(d);
    }
    depends_.clear();
}

void coarse_task::commit(op_ptr&& op) {
    DEBUG_FMT("this={} {}(op={})", format::ptr(this), __func__, format::ptr(op.get()));

    if (!kernel_) {
        kernel_ = make_op_task(make_kernel_op());
        DEBUG_FMT("\tkernel_={}", format::ptr(kernel_.get()));
    }

    auto task = make_op_task(std::move(op));

    for (auto& d : depends_) {
        DEBUG_FMT("\ttask {} runs after {}", format::ptr(task), format::ptr(d.get()));
        task->runs_after(d);
    }
    depends_.clear();

    DEBUG_FMT("\ttask {} runs after {}", format::ptr(kernel_.get()), format::ptr(task.get()));
    kernel_->runs_after(task);
    task->finalize();
}

void coarse_task::commit_as_core(op_ptr&& op) {
    DEBUG_FMT("this={} {}(op={})", format::ptr(this), __func__, format::ptr(op.get()));

    if (kernel_) {
        kernel_->override_op(std::move(op));
    } else {
        kernel_ = make_op_task(std::move(op));
    }

    DEBUG_FMT("\tkernel_={}", format::ptr(kernel_.get()));
}

std::unique_ptr<rts::event> coarse_task::submit() {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    if (!kernel_) {
        if (k_.name || k_.desc || k_.name) {
            kernel_ = make_op_task(make_kernel_op());
            DEBUG_FMT("kernel_={}", format::ptr(kernel_.get()));
        } else {
            kernel_ = make_nop_task();
            DEBUG_FMT("kernel_={} NOP", format::ptr(kernel_.get()));
        }
    }

    commit();

    kernel_->finalize();
    kernel_weak_ = std::weak_ptr(kernel_);
    kernel_.reset();

    return std::make_unique<event>(kernel_weak_);
}

void coarse_task::enable_profiling() {}

task_ptr coarse_task::lock_kernel_task() const {
    if (kernel_) {
        return kernel_;
    }

    return kernel_weak_.lock();
}

void coarse_task::depends_on(coarse_task const& task) {
    DEBUG_FMT("this={} {}(task={})", format::ptr(this), __func__, format::ptr(&task));

    if (auto pre = task.lock_kernel_task()) {
        depends_.push_back(pre);
    }
}

void coarse_task::depends_on(std::shared_ptr<rts::task> const& task) {
    DEBUG_FMT("this={} {}(task={})", format::ptr(this), __func__, format::ptr(task.get()));

    depends_on(*std::static_pointer_cast<coarse_task>(task));
}

void coarse_task::depends_on(rts::event const& ev) {
    DEBUG_FMT("this={} {}(ev={})", format::ptr(this), __func__, format::ptr(&ev));

    if (auto ev_task = static_cast<event const&>(ev).lock()) {
        depends_.push_back(ev_task);
    }
}

void coarse_task::use_device() {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.is_device = true;
}

void coarse_task::set_device(rts::device& dev) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.device = &dev;
}

void coarse_task::use_host() {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.is_device = false;
}

void coarse_task::set_kernel(char const* name, uint32_t hash) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.name = name;
    k_.hash = hash;
}

void coarse_task::set_host(std::function<void()> const& f) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.host_fn = f;
}

void coarse_task::copy_1d(rts::buffer& src, size_t src_off_byte, rts::buffer& dst,
                          size_t dst_off_byte, size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(make_copy_1d_op(src, src_off_byte, dst, dst_off_byte, len_byte));
}

void coarse_task::copy_1d(rts::buffer& src, size_t src_off_byte, void* dst, size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(make_copy_1d_op(src, src_off_byte, dst, len_byte));
}

void coarse_task::copy_1d(void const* src, rts::buffer& dst, size_t dst_off_byte,
                          size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(make_copy_1d_op(src, dst, dst_off_byte, len_byte));
}

void coarse_task::copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride,
                          rts::buffer& dst, size_t dst_off_byte, size_t dst_stride, size_t loop,
                          size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(make_copy_2d_op(src, src_off_byte, src_stride, dst, dst_off_byte, dst_stride,
                                   loop, len_byte));
}

void coarse_task::copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, void* dst,
                          size_t dst_stride, size_t loop, size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(
        make_copy_2d_op(src, src_off_byte, src_stride, dst, dst_stride, loop, len_byte));
}

void coarse_task::copy_2d(void const* src, size_t src_stride, rts::buffer& dst,
                          size_t dst_off_byte, size_t dst_stride, size_t loop,
                          size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(
        make_copy_2d_op(src, src_stride, dst, dst_off_byte, dst_stride, loop, len_byte));
}

void coarse_task::copy_3d(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                          size_t j_src_stride, rts::buffer& dst, size_t dst_off_byte,
                          size_t i_dst_stride, size_t j_dst_stride, size_t i_loop,
                          size_t j_loop, size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(make_copy_3d_op(src, src_off_byte, i_src_stride, j_src_stride, dst,
                                   dst_off_byte, i_dst_stride, j_dst_stride, i_loop, j_loop,
                                   len_byte));
}

void coarse_task::copy_3d(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                          size_t j_src_stride, void* dst, size_t i_dst_stride,
                          size_t j_dst_stride, size_t i_loop, size_t j_loop, size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(make_copy_3d_op(src, src_off_byte, i_src_stride, j_src_stride, dst,
                                   i_dst_stride, j_dst_stride, i_loop, j_loop, len_byte));
}

void coarse_task::copy_3d(void const* src, size_t i_src_stride, size_t j_src_stride,
                          rts::buffer& dst, size_t dst_off_byte, size_t i_dst_stride,
                          size_t j_dst_stride, size_t i_loop, size_t j_loop, size_t len_byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(make_copy_3d_op(src, i_src_stride, j_src_stride, dst, dst_off_byte,
                                   i_dst_stride, j_dst_stride, i_loop, j_loop, len_byte));
}

void coarse_task::fill(rts::buffer& dst, size_t byte_len) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    commit_as_core(make_fill_op(dst, byte_len));
}

void coarse_task::set_desc(rts::func_desc const* desc) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.desc = desc;
}

void coarse_task::set_single() {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.is_single = true;
    k_.is_ndr = false;
    k_.range.fill(1);
}

void coarse_task::set_range(rts::range const& range) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.is_single = false;
    k_.is_ndr = false;
    for (int i = 0; i < 3; i++) {
        k_.range[i] = range.size[i];
    }
    for (int i = 3; i < 6; i++) {
        k_.range[i] = 0;
    }
}

void coarse_task::set_nd_range(rts::nd_range const& ndr) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.is_single = false;
    k_.is_ndr = true;
    k_.range[0] = ndr.global[0] / ndr.local[0];
    k_.range[1] = ndr.global[1] / ndr.local[1];
    k_.range[2] = ndr.global[2] / ndr.local[2];
    k_.range[3] = ndr.local[0];
    k_.range[4] = ndr.local[1];
    k_.range[5] = ndr.local[2];
}

void coarse_task::set_local_mem_size(size_t byte) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.lmem = byte;
}

void coarse_task::set_param(void const* param, size_t size) {
    DEBUG_FMT("this={} {}()", format::ptr(this), __func__);

    k_.add_param(param, size);
}

void coarse_task::kernel_desc::add_param(void const* param, size_t byte) {
    add_param(param, byte, alignof(std::max_align_t));
}

void coarse_task::kernel_desc::add_param(void const* param, size_t size_of, size_t align_of) {
    auto addr = reinterpret_cast<uintptr_t>(this->next_param);
    auto const mask = static_cast<uintptr_t>(align_of) - 1;

    if (addr & mask) {
        addr = (addr & ~mask) + align_of;
    }
    auto const ptr = reinterpret_cast<std::byte*>(addr);

    DEBUG_FMT("this={} {}(param={}, size_of={}, align_of={}) => {} 0x{:x}",
              reinterpret_cast<void const*>(
                  reinterpret_cast<uintptr_t>(this) -
                  reinterpret_cast<uintptr_t>(&reinterpret_cast<coarse_task const*>(0)->k_)),
              __func__, format::ptr(param), size_of, align_of, this->n_args, addr);

    memcpy(ptr, param, size_of);
    add_param_direct(ptr);

    this->next_param = reinterpret_cast<std::byte*>(addr + size_of);
}

void coarse_task::kernel_desc::add_param_direct(void* ptr) {
    this->args[this->n_args++] = ptr;
}

}  // namespace dev_rts
CHARM_SYCL_END_NAMESPACE
