#pragma once

#if defined(NDEBUG) && !defined(CHARM_SYCL_ENABLE_LOGGING)
#else
#    include <cstdint>
#    include <cstdlib>
#    include <cstring>
#    include <fmt/compile.h>
#    include <fmt/format.h>
#endif

namespace utils::logging {

#if defined(NDEBUG) && !defined(CHARM_SYCL_ENABLE_LOGGING)
inline bool global_enabled() {
    return false;
}

inline bool scope_enabled(char const*, size_t) {
    return false;
}

inline uint64_t timer_now() {
    return 0;
}

inline void timer_reset() {}
#else
bool global_enabled();

bool scope_enabled(char const* name, size_t name_len);

uint64_t timer_now();

void timer_reset();
#endif

}  // namespace utils::logging

#if defined(NDEBUG) && !defined(CHARM_SYCL_ENABLE_LOGGING)
#    define LOGGING_DEFINE_SCOPE(scope)

#    define init_logging()

#    define DEBUG_FMT(fmtstr, ...)

#    define DEBUG_LOG(msg)
#else
#    define LOGGING_DEFINE_SCOPE(scope)                                                     \
        [[maybe_unused]] static bool logging_enabled = false;                               \
        [[maybe_unused]] static constexpr auto const* logging_scope = #scope;               \
        [[maybe_unused]] inline static void init_logging() {                                \
            logging_enabled = ::utils::logging::scope_enabled(#scope, std::strlen(#scope)); \
        }

#    define DEBUG_FMT(fmtstr, ...)                                                          \
        ({                                                                                  \
            if (logging_enabled) {                                                          \
                auto const t = ::utils::logging::timer_now() / 1e3;                         \
                fmt::print(FMT_COMPILE("[{:13.3f}][{:6s}] " fmtstr "\n"), t, logging_scope, \
                           __VA_ARGS__);                                                    \
            }                                                                               \
        })

#    define DEBUG_LOG(msg) DEBUG_FMT("{}", (msg))
#endif
