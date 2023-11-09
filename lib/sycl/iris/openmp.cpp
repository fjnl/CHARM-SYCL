#include <array>
#include <cstring>
#include <mutex>
#include <assert.h>
#include <iris/iris.h>
#include "../dev_rts.hpp"
#include "../kreg.hpp"
#include "../rts.hpp"

namespace {

namespace rts = CHARM_SYCL_NS::rts;

using dev_fn_t = void (*)(void**);

static std::mutex g_mutex;
static std::unique_lock<std::mutex> g_lock;
static dev_rts::task_parameter_storage g_args;
static rts::range g_range(0, 0, 0);
static dev_fn_t g_fn;
static constexpr auto CPU_C = std::string_view("cpu-c");
static constexpr auto CPU_C_H = sycl::detail::fnv1a("cpu-c");
static constexpr auto CPU_OPENMP = std::string_view("cpu-openmp");
static constexpr auto CPU_OPENMP_H = sycl::detail::fnv1a("cpu-openmp");

}  // namespace

extern "C" int iris_openmp_init() {
    return IRIS_SUCCESS;
}

extern "C" int iris_openmp_finalize() {
    return IRIS_SUCCESS;
}

extern "C" int iris_openmp_kernel(const char* name) {
    std::unique_lock lk(g_mutex);

    auto& reg = kreg::get();
    auto fn = reg.find(name, sycl::detail::fnv1a(name), CPU_OPENMP, CPU_OPENMP_H);

    if (!fn) {
        fn = reg.find(name, sycl::detail::fnv1a(name), CPU_C, CPU_C_H);
    }

    if (!fn) {
        return IRIS_ERROR;
    }

    g_fn = reinterpret_cast<dev_fn_t>(fn);
    g_lock = std::move(lk);
    g_args.clear();

    return IRIS_SUCCESS;
}

extern "C" int iris_openmp_setarg(int, size_t size, void* value) {
    auto* ptr = g_args.next_param_ptr(size);
    if (!ptr) {
        return IRIS_ERROR;
    }

    std::memcpy(ptr, value, size);

    return IRIS_SUCCESS;
}

extern "C" int iris_openmp_setmem(int, void* mem) {
    auto* ptr = g_args.next_param_ptr<void*>();
    if (!ptr) {
        return IRIS_ERROR;
    }

    *ptr = mem;

    return IRIS_SUCCESS;
}

extern "C" int iris_openmp_launch([[maybe_unused]] int dim, [[maybe_unused]] size_t off,
                                  [[maybe_unused]] size_t gws) {
    auto lk = std::move(g_lock);

    if (off != 0) {
        // TODO:
        fprintf(stderr, "Not implemented: %s: %d\n", __FILE__, __LINE__);
        return IRIS_ERROR;
    }

    g_fn(g_args.data());

    return IRIS_SUCCESS;
}
