#include <memory>
#include <mutex>
#include <vector>
#include <boost/context/fiber.hpp>
#include <stdarg.h>
#include <utils/logging.hpp>
#ifdef HAVE_GET_NPROCS
#    include <sys/sysinfo.h>
#endif
#include "fiber.hpp"

extern "C" void __charm_sycl_fiber_barrier(void* ctx);

extern "C" void __charm_sycl_fiber_fork(int ndim, unsigned long g0, unsigned long g1,
                                        unsigned long g2, unsigned long l_range0,
                                        unsigned long l_range1, unsigned long l_range2,
                                        unsigned int* lmem_size, void* fn, ...);

namespace {

LOGGING_DEFINE_SCOPE(fiber)

using fiber = boost::context::fiber;

using kernel_func_1d_t = void (*)(void* lmem, void* ctx, unsigned long g0, unsigned long l0,
                                  ::va_list args);

using kernel_func_2d_t = void (*)(void* lmem, void* ctx, unsigned long g0, unsigned long g1,
                                  unsigned long l0, unsigned long l1, ::va_list args);

using kernel_func_3d_t = void (*)(void* lmem, void* ctx, unsigned long g0, unsigned long g1,
                                  unsigned long g2, unsigned long l0, unsigned long l1,
                                  unsigned long l2, ::va_list args);

struct release_by_free {
    void operator()(void* ptr) const {
        ::free(ptr);
    }
};

std::unique_ptr<char, release_by_free> aligned_malloc(size_t byte) {
    void* ptr = nullptr;
    ::posix_memalign(&ptr, 4096, byte);
    return std::unique_ptr<char, release_by_free>(reinterpret_cast<char*>(ptr));
}

struct work_group;
struct work_item;

struct work_item {
    friend struct work_group;

    void syncthreads();

    uint32_t id0() const {
        return id0_;
    }

    uint32_t id1() const {
        return id1_;
    }

    uint32_t id2() const {
        return id2_;
    }

private:
    explicit work_item(work_group* wg, fiber* prev, fiber* next, uint32_t id0, uint32_t id1,
                       uint32_t id2)
        : wg_(wg), prev_(prev), next_(next), id0_(id0), id1_(id1), id2_(id2) {}

    void syncthreads_not_run();

    [[maybe_unused]] work_group* wg_;
    fiber* prev_;
    fiber* next_;
    uint32_t id0_, id1_, id2_;
};

struct work_group {
    friend struct work_item;

    void alloc_lmem(size_t byte) {
        if (lmem_size_ < byte) {
            ptr_ = aligned_malloc(byte);
            lmem_ = ptr_.get();
            lmem_size_ = byte;
        }
    }

    void alloc_threads(unsigned n) {
        nt_ = n;
        if (threads_.size() < n) {
            threads_.reserve(n);
            while (threads_.size() < n) {
                add_thread(threads_.size());
            }
        }
    }

    template <class F>
    void set_func(unsigned n, F&& f) {
        alloc_threads(n);
        fn_ = std::forward<F>(f);
        running_ = true;
    }

    bool resume() {
        assert(this->running_);
        threads_[nt_ - 1] = std::move(threads_[0]).resume();
        return running_;
    }

    void* lmem() {
        return lmem_;
    }

private:
    void add_thread(unsigned i) {
        threads_.emplace_back([i, this](fiber&& f) -> fiber {
            work_item wi(this, i == 0 ? &tail_ : &threads_[i - 1], nullptr, 0, 0, i);

            *wi.prev_ = std::move(f);

            for (;;) {
                wi.next_ = i == nt_ - 1 ? &tail_ : &threads_[i + 1];

                fn_(this, &wi);

                if (i == 0) {
                    running_ = false;
                }
                wi.syncthreads_not_run();
            }
        });
    }

    void* lmem_;
    size_t lmem_size_ = 0;
    fiber tail_;
    unsigned nt_ = 0;
    std::vector<fiber> threads_;
    std::function<void(work_group*, work_item*)> fn_;
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
        wg->alloc_threads(512);
        wg->alloc_lmem(256 * 1024);
        buff.push_back(std::move(wg));
    }

    for (auto& wg : buff) {
        release_wg(std::move(wg));
    }

    DEBUG_LOG("init: end");
}

}  // namespace runtime::impl
CHARM_SYCL_END_NAMESPACE

void __charm_sycl_fiber_barrier(void* ctx) {
    auto wi = reinterpret_cast<work_item*>(ctx);
    wi->syncthreads();
}

void __charm_sycl_fiber_fork(int ndim, unsigned long g0, [[maybe_unused]] unsigned long g1,
                             [[maybe_unused]] unsigned long g2, unsigned long l_range0,
                             unsigned long l_range1, unsigned long l_range2,
                             unsigned int* lmem_size, void* fn, ...) {
    DEBUG_FMT(
        "__charm_sycl_fiber_fork(ndim={}, G=[{}, {}, {}], L=[{}, {}, {}], size={}, fn={}, ...)",
        ndim, g0, g1, g2, l_range0, l_range1, l_range2, *lmem_size, fmt::ptr(fn));

    auto wg = acquire_wg();

    ::va_list ap;
    va_start(ap, fn);

    wg->alloc_lmem(*lmem_size);

    if (ndim == 1) {
        wg->set_func(
            l_range0 * l_range1 * l_range2, [fn, &ap, g0](work_group* wg, work_item* wi) {
                va_list ap_copy;
                va_copy(ap_copy, ap);
                reinterpret_cast<kernel_func_1d_t>(fn)(wg->lmem(), wi, g0, wi->id2(), ap_copy);
                va_end(ap_copy);
            });
    } else {
        // TODO
        std::abort();
    }

    DEBUG_LOG("wg initialized.");

#if !defined(NDEBUG) || defined(CHARM_SYCL_ENABLE_LOGGING)
    if (logging_enabled) [[unlikely]] {
        auto const t_start = ::utils::logging::timer_now();
        while (wg->resume()) {
        }
        auto const t_end = ::utils::logging::timer_now();
        DEBUG_FMT("wg completed: {:3f} us", (t_end - t_start) / 1.0e3);
    } else
#endif
    {
        while (wg->resume()) {
        }
    }

    va_end(ap);

    release_wg(std::move(wg));

    DEBUG_LOG("wg released.");
}
