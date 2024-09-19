#include <memory>
#include <mutex>
#include <vector>
#include <boost/context/fiber.hpp>
#include <boost/context/protected_fixedsize_stack.hpp>
#include <stdarg.h>
#ifdef HAVE_GET_NPROCS
#    include <sys/sysinfo.h>
#endif
#include "fiber.hpp"
#include "logging.hpp"

extern "C" void* __charm_sycl_fiber_memory();

extern "C" void __charm_sycl_fiber_barrier();

extern "C" unsigned long __charm_sycl_fiber_group_range3();
extern "C" unsigned long __charm_sycl_fiber_group_id3();
extern "C" unsigned long __charm_sycl_fiber_local_range3();
extern "C" unsigned long __charm_sycl_fiber_local_id3();

extern "C" unsigned long __charm_sycl_fiber_group_range2();
extern "C" unsigned long __charm_sycl_fiber_group_id2();
extern "C" unsigned long __charm_sycl_fiber_local_range2();
extern "C" unsigned long __charm_sycl_fiber_local_id2();

extern "C" unsigned long __charm_sycl_fiber_group_range1();
extern "C" unsigned long __charm_sycl_fiber_group_id1();
extern "C" unsigned long __charm_sycl_fiber_local_range1();
extern "C" unsigned long __charm_sycl_fiber_local_id1();

namespace {

LOGGING_DEFINE_SCOPE(fiber)

using fiber = boost::context::fiber;

struct release_by_free {
    void operator()(void* ptr) const {
        ::free(ptr);
    }
};

std::unique_ptr<char, release_by_free> aligned_malloc(size_t byte) {
    void* ptr = nullptr;
    ::posix_memalign(&ptr, 256 * 1024, byte);
    return std::unique_ptr<char, release_by_free>(reinterpret_cast<char*>(ptr));
}

struct work_group;
struct work_item;

thread_local work_group* current_wg = nullptr;
thread_local work_item* current_wi = nullptr;

struct work_item {
    friend struct work_group;

    explicit work_item(work_group* wg) : wg_(wg) {}

    work_item(work_item const&) = delete;
    work_item& operator=(work_item const&) = delete;
    work_item(work_item&&) = default;
    work_item& operator=(work_item&&) = default;

    void syncthreads();

    size_t local_id1() const {
        return local_id_[0];
    }

    size_t local_id2() const {
        return local_id_[1];
    }

    size_t local_id3() const {
        return local_id_[2];
    }

    void set_id(size_t lid1, size_t lid2, size_t lid3) {
        local_id_[0] = lid1;
        local_id_[1] = lid2;
        local_id_[2] = lid3;
    }

private:
    explicit work_item(work_group* wg, fiber* prev, fiber* next, size_t local_id1,
                       size_t local_id2, size_t local_id3)
        : wg_(wg), prev_(prev), next_(next), local_id_{{local_id1, local_id2, local_id3}} {}

    void syncthreads_not_run();

    [[maybe_unused]] work_group* wg_ = nullptr;
    fiber* prev_ = nullptr;
    fiber* next_ = nullptr;
    std::array<size_t, 3> local_id_{{0, 0, 0}};
};

struct work_group {
    friend struct work_item;

    void set_lmem(size_t byte) {
        if (lmem_size_ < byte) {
            ptr_ = aligned_malloc(byte);
            lmem_ = ptr_.get();
            lmem_size_ = byte;
        }
    }

    size_t group_id1() const {
        return group_id_[0];
    }

    size_t group_id2() const {
        return group_id_[1];
    }

    size_t group_id3() const {
        return group_id_[2];
    }

    size_t group_range1() const {
        return group_range_[0];
    }

    size_t group_range2() const {
        return group_range_[1];
    }

    size_t group_range3() const {
        return group_range_[2];
    }

    size_t local_range1() const {
        return local_range_[0];
    }

    size_t local_range2() const {
        return local_range_[1];
    }

    size_t local_range3() const {
        return local_range_[2];
    }

    void set_group_range(size_t group_range1, size_t group_range2, size_t group_range3) {
        group_range_[0] = group_range1;
        group_range_[1] = group_range2;
        group_range_[2] = group_range3;
    }

    void set_group_id(size_t group_id1, size_t group_id2, size_t group_id3) {
        group_id_[0] = group_id1;
        group_id_[1] = group_id2;
        group_id_[2] = group_id3;
    }

    void set_local_range(size_t local_range1, size_t local_range2, size_t local_range3) {
        auto const n_items = local_range1 * local_range2 * local_range3;

        alloc_threads(n_items);
        n_threads_ = n_items;

        for (size_t i = 0, idx = 0; i < local_range1; i++) {
            for (size_t j = 0; j < local_range2; j++) {
                for (size_t k = 0; k < local_range3; k++, idx++) {
                    work_items_[idx].set_id(i, j, k);
                }
            }
        }

        local_range_[0] = local_range1;
        local_range_[1] = local_range2;
        local_range_[2] = local_range3;
    }

    template <class F>
    void set_func(F&& f) {
        fn_ = std::forward<F>(f);
        running_ = true;
    }

    bool resume() {
        assert(this->running_);
        threads_[n_threads_ - 1] = std::move(threads_[0]).resume();
        return running_;
    }

    void* lmem() {
        return lmem_;
    }

private:
    void alloc_threads(unsigned n) {
        if (threads_.size() < n) {
            threads_.reserve(n);
            work_items_.reserve(n);

            while (threads_.size() < n) {
                add_thread(threads_.size());
                work_items_.emplace_back(this);
            }
        }
    }

    void add_thread(unsigned i) {
        threads_.emplace_back(std::allocator_arg, boost::context::fixedsize_stack(1024 * 1024),
                              [i, this](fiber&& f) -> fiber {
                                  work_item* wi = &work_items_[i];

                                  wi->prev_ = i == 0 ? &tail_ : &threads_[i - 1];
                                  *wi->prev_ = std::move(f);

                                  for (;;) {
                                      wi = &work_items_[i];
                                      wi->next_ =
                                          i == n_threads_ - 1 ? &tail_ : &threads_[i + 1];
                                      current_wg = this;
                                      current_wi = wi;

                                      fn_();

                                      if (i == 0) {
                                          running_ = false;
                                      }
                                      wi->syncthreads_not_run();
                                  }
                              });
    }

    void* lmem_;
    size_t lmem_size_ = 0;
    fiber tail_;
    std::array<size_t, 3> group_range_;
    std::array<size_t, 3> group_id_;
    std::array<size_t, 3> local_range_;
    std::vector<fiber> threads_;
    std::vector<work_item> work_items_;
    std::function<void()> fn_;
    unsigned int n_threads_ = 0;
    bool running_ = false;
    std::unique_ptr<void, release_by_free> ptr_;
};

void work_item::syncthreads() {
    assert(wg_->running_);
    *prev_ = std::move(*next_).resume();
}

void work_item::syncthreads_not_run() {
    assert(!wg_->running_);
    *prev_ = std::move(*next_).resume();
}

std::mutex g_lock;
std::vector<std::unique_ptr<work_group>> g_wg_cache;

std::unique_ptr<work_group> acquire_wg() {
    std::unique_lock lk(g_lock);

    if (!g_wg_cache.empty()) {
        auto wg = std::move(g_wg_cache.back());

        g_wg_cache.pop_back();

        return wg;
    }

    lk.unlock();

    return std::make_unique<work_group>();
}

void release_wg(std::unique_ptr<work_group> wg) {
    std::unique_lock lk(g_lock);

    g_wg_cache.push_back(std::move(wg));
}

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime::impl {

void fiber_init() {
    init_logging();

    DEBUG_LOG("init: start");

#ifdef HAVE_GET_NPROCS
    auto const np = get_nprocs();
#else
    auto const np = 1;
#endif

    DEBUG_FMT("np={}", np);

    std::vector<std::unique_ptr<work_group>> buff;
    for (int i = 0; i < np; i++) {
        auto wg = acquire_wg();
        wg->set_local_range(1, 1, 512);
        wg->set_lmem(256 * 1024);
        buff.push_back(std::move(wg));
    }

    for (auto& wg : buff) {
        release_wg(std::move(wg));
    }

    DEBUG_LOG("init: end");
}

void exec_with_fibers(size_t group_range1, size_t group_range2, size_t group_range3,
                      size_t local_range1, size_t local_range2, size_t local_range3,
                      size_t lmem_byte, std::function<void(void**)> const& fn, void** args) {
    DEBUG_FMT("{}(group_range={}, {}, {}, local_range={}, {}, {}, lmem_byte={})", __func__,
              group_range1, group_range2, group_range3, local_range1, local_range2,
              local_range3, lmem_byte);

    // #pragma omp parallel for collapse(3)
    for (size_t i = 0; i < group_range1; i++) {
        for (size_t j = 0; j < group_range2; j++) {
            for (size_t k = 0; k < group_range3; k++) {
                auto wg = acquire_wg();

                wg->set_group_range(group_range1, group_range2, group_range3);
                wg->set_group_id(i, j, k);
                wg->set_local_range(local_range1, local_range2, local_range3);
                wg->set_lmem(lmem_byte);
                wg->set_func([&]() {
                    fn(args);
                });

                while (wg->resume()) {
                }

                release_wg(std::move(wg));
            }
        }
    }
}

}  // namespace runtime::impl
CHARM_SYCL_END_NAMESPACE

void* __charm_sycl_fiber_memory() {
    return current_wg->lmem();
}

void __charm_sycl_fiber_barrier() {
    auto* wi = current_wi;
    current_wi = nullptr;
    wi->syncthreads();
    current_wi = wi;
}

unsigned long __charm_sycl_fiber_group_range3() {
    return current_wg->group_range3();
}

unsigned long __charm_sycl_fiber_group_id3() {
    return current_wg->group_id3();
}

unsigned long __charm_sycl_fiber_local_range3() {
    return current_wg->local_range3();
}

unsigned long __charm_sycl_fiber_local_id3() {
    return current_wi->local_id3();
}

unsigned long __charm_sycl_fiber_group_range2() {
    return current_wg->group_range2();
}

unsigned long __charm_sycl_fiber_group_id2() {
    return current_wg->group_id2();
}

unsigned long __charm_sycl_fiber_local_range2() {
    return current_wg->local_range2();
}

unsigned long __charm_sycl_fiber_local_id2() {
    return current_wi->local_id2();
}

unsigned long __charm_sycl_fiber_group_range1() {
    return current_wg->group_range1();
}

unsigned long __charm_sycl_fiber_group_id1() {
    return current_wg->group_id1();
}

unsigned long __charm_sycl_fiber_local_range1() {
    return current_wg->local_range1();
}

unsigned long __charm_sycl_fiber_local_id1() {
    return current_wi->local_id1();
}
