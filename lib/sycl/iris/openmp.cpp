#include <array>
#include <cstring>
#include <mutex>
#include <assert.h>
#include "../dev_rts.hpp"
#include "../fiber.hpp"
#include "../kreg.hpp"
#include "../rts.hpp"

#define SUCCESS 0
#define ERROR -1

namespace {

namespace rts = CHARM_SYCL_NS::rts;
namespace impl = CHARM_SYCL_NS::runtime::impl;

using dev_fn_t = void (*)(void**);

static std::mutex g_mutex;
static std::unique_lock<std::mutex> g_lock;
static dev_rts::task_parameter_storage g_args;
static rts::range g_range(0, 0, 0);
static kreg::kernel_info const* g_kinfo;
static void const* g_last_arg;
static constexpr auto CPU_C = std::string_view("cpu-c");
static constexpr auto CPU_C_H = sycl::detail::fnv1a("cpu-c");
static constexpr auto CPU_OPENMP = std::string_view("cpu-openmp");
static constexpr auto CPU_OPENMP_H = sycl::detail::fnv1a("cpu-openmp");

}  // namespace

extern "C" int iris_openmp_init() {
    return SUCCESS;
}

extern "C" int iris_openmp_finalize() {
    return SUCCESS;
}

extern "C" int iris_openmp_kernel(const char* name) {
    std::unique_lock lk(g_mutex);

    auto& reg = kreg::get();
    auto info = reg.find(name, sycl::detail::fnv1a(name), CPU_OPENMP, CPU_OPENMP_H);

    if (!info) {
        info = reg.find(name, sycl::detail::fnv1a(name), CPU_C, CPU_C_H);
    }

    if (!info) {
        return ERROR;
    }

    g_kinfo = info;
    g_lock = std::move(lk);
    g_args.clear();
    g_last_arg = nullptr;

    return SUCCESS;
}

extern "C" int iris_openmp_setarg(int, size_t size, void* value) {
    auto* ptr = g_args.next_param_ptr(size);
    if (!ptr) {
        return ERROR;
    }

    std::memcpy(ptr, value, size);
    g_last_arg = ptr;

    return SUCCESS;
}

extern "C" int iris_openmp_setmem(int, void* mem) {
    auto* ptr = g_args.next_param_ptr<void*>();
    if (!ptr) {
        return ERROR;
    }

    *ptr = mem;
    g_last_arg = ptr;

    return SUCCESS;
}

extern "C" int iris_openmp_launch([[maybe_unused]] int dim, [[maybe_unused]] size_t off,
                                  [[maybe_unused]] size_t const* gws) {
    auto lk = std::move(g_lock);

    auto fn = reinterpret_cast<dev_fn_t>(g_kinfo->fn);

    if (g_kinfo->is_ndr) {
        // FIXME: unsafe:
        // This will be UB if the memory layout of the struct iris::Command is changed.
        size_t const* lws = &gws[3];
        impl::exec_with_fibers(gws[0] / lws[0], gws[1] / lws[1], gws[2] / lws[2], lws[0],
                               lws[1], lws[2], 256 * 1024, fn, g_args.data());
    } else {
        fn(g_args.data());
    }

    return SUCCESS;
}
