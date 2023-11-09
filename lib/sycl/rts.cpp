#include <string_view>
#include <fmt/format.h>
#include <stdlib.h>
#include <strings.h>
#include "rt.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace rts {

std::unique_ptr<subsystem> make_subsystem() {
    auto env = getenv("CHARM_SYCL_RTS");

    if (env) {
        if (strcasecmp(env, "dev") == 0 || strcasecmp(env, "CPU") == 0) {
            return runtime::impl::make_dev_rts();
        }
#ifdef HAVE_DEV_RTS_CUDA
        else if (strcasecmp(env, "dev-cuda") == 0 || strcasecmp(env, "cuda") == 0) {
            return runtime::impl::make_dev_rts_cuda();
        }
#endif

#ifdef HAVE_DEV_RTS_HIP
        else if (strcasecmp(env, "dev-hip") == 0 || strcasecmp(env, "hip") == 0) {
            return runtime::impl::make_dev_rts_hip();
        }
#endif

#if HAVE_IRIS
        else if (strcasecmp(env, "iris") == 0) {
            return runtime::impl::make_iris_rts();
        }
#endif

        auto const errmsg = fmt::format("Unknown RTS Name: {} (from CHARM_SYCL_RTS)\n", env);
        throw std::runtime_error(errmsg);
    }

#if defined(HAVE_IRIS)
    return runtime::impl::make_iris_rts();
#else
    return runtime::impl::make_dev_rts();
#endif
}

}  // namespace rts

CHARM_SYCL_END_NAMESPACE
