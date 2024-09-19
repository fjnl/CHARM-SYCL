#include <string_view>
#include <stdlib.h>
#include <strings.h>
#include "blas/blas.hpp"
#include "format.hpp"
#include "interfaces.hpp"
#include "logging.hpp"
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace rts {

static std::unique_ptr<subsystem> make_subsystem_impl() {
    auto env = getenv("CHARM_SYCL_RTS");

    if (env) {
        if (strcasecmp(env, "dev") == 0 || strcasecmp(env, "CPU") == 0) {
            INFO("CPU RTS is loaded.");
            blas::init_blas_cpu();
            return runtime::impl::make_dev_rts();
        }
        if (strcasecmp(env, "dev-cuda") == 0 || strcasecmp(env, "cuda") == 0) {
            auto p = unwrap(runtime::impl::make_dev_rts_cuda());
            INFO("CUDA RTS is loaded.");
            blas::init_blas_cuda();
            return p;
        }
        if (strcasecmp(env, "dev-hip") == 0 || strcasecmp(env, "hip") == 0) {
            auto p = unwrap(runtime::impl::make_dev_rts_hip());
            INFO("HIP RTS is loaded.");
            blas::init_blas_hip();
            return p;
        }
        if (strcasecmp(env, "iris") == 0 || strcasecmp(env, "iris-dmem") == 0 ||
            strcasecmp(env, "iris_dmem") == 0 || strcasecmp(env, "iris dmem") == 0) {
            auto p = unwrap(runtime::impl::make_iris_dmem_rts());
            INFO("IRIS RTS (DMEM) is loaded.");
            blas::init_blas_cpu();
            blas::init_blas_cuda();
            blas::init_blas_hip();
            return p;
        }

        if (strcasecmp(env, "iris-old") == 0 || strcasecmp(env, "iris_old") == 0 ||
            strcasecmp(env, "iris old") == 0 || strcasecmp(env, "iris-explicit") == 0 ||
            strcasecmp(env, "iris_explicit") == 0 || strcasecmp(env, "iris explicit") == 0) {
            auto p = unwrap(runtime::impl::make_iris_rts());
            INFO("IRIS RTS (explicit) is loaded.");
            blas::init_blas_cpu();
            blas::init_blas_cuda();
            blas::init_blas_hip();
            return p;
        }

        format::print(std::cerr, "Unknown RTS Name: {} (from CHARM_SYCL_RTS)\n", env);
        std::exit(1);
    }

    /* If no RTS is given */

    if (auto iris = runtime::impl::make_iris_rts()) {
        INFO("IRIS RTS (explicit) is loaded.");
        blas::init_blas_cpu();
        blas::init_blas_cuda();
        blas::init_blas_hip();
        return std::move(iris).value();
    }
    if (auto cuda = runtime::impl::make_dev_rts_cuda()) {
        INFO("CUDA RTS is loaded.");
        blas::init_blas_cuda();
        return std::move(cuda).value();
    }
    if (auto hip = runtime::impl::make_dev_rts_hip()) {
        INFO("HIP RTS is loaded.");
        blas::init_blas_hip();
        return std::move(hip).value();
    }

    auto p = runtime::impl::make_dev_rts();
    if (p) {
        INFO("CPU RTS is loaded.");
        blas::init_blas_cpu();
    }
    return p;
}

std::unique_ptr<subsystem> make_subsystem() {
    logging::g_init_logging();

    auto rts = make_subsystem_impl();

    return rts;
}

}  // namespace rts

CHARM_SYCL_END_NAMESPACE
