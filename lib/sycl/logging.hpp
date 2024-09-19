#pragma once

#include <charm/sycl/config.hpp>
#include "format.hpp"

CHARM_SYCL_BEGIN_NAMESPACE
namespace logging {

extern bool info_enable;
extern bool warn_enable;

void g_init_logging();

void info(std::string_view msg);
void warn(std::string_view msg);
[[noreturn]] void fatal(std::string_view msg);

template <class... Args>
void info(format::format_string<Args...> fmt, Args&&... args) {
    info(format::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void warn(format::format_string<Args...> fmt, Args&&... args) {
    warn(format::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
[[noreturn]] void fatal(format::format_string<Args...> fmt, Args&&... args) {
    fatal(format::format(fmt, std::forward<Args>(args)...));
}

#define INFO(...)                                               \
    ({                                                          \
        if (CHARM_SYCL_NS::logging::info_enable) [[unlikely]] { \
            CHARM_SYCL_NS::logging::info(__VA_ARGS__);          \
        }                                                       \
    })

#define WARN(...)                                               \
    ({                                                          \
        if (CHARM_SYCL_NS::logging::warn_enable) [[unlikely]] { \
            CHARM_SYCL_NS::logging::warn(__VA_ARGS__);          \
        }                                                       \
    })

#define FATAL(...) ({ CHARM_SYCL_NS::logging::fatal(__VA_ARGS__); })

}  // namespace logging
CHARM_SYCL_END_NAMESPACE

#if defined(NDEBUG) && !defined(CHARM_SYCL_ENABLE_LOGGING)
#else
#    include <cstdint>
#    include <cstdlib>
#    include <cstring>
#    include "format.hpp"
#endif

CHARM_SYCL_BEGIN_NAMESPACE
namespace logging {

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

}  // namespace logging
CHARM_SYCL_END_NAMESPACE

#if defined(NDEBUG) && !defined(CHARM_SYCL_ENABLE_LOGGING)
#    define LOGGING_DEFINE_SCOPE(scope)

#    define init_logging()

#    define DEBUG_FMT(fmtstr, ...)

#    define DEBUG_LOG(msg)

#    undef CHARM_SYCL_ENABLE_LOGGING
#else
#    define LOGGING_DEFINE_SCOPE(scope)                                             \
        [[maybe_unused]] static bool logging_enabled = false;                       \
        [[maybe_unused]] static constexpr auto const* logging_scope = #scope;       \
        [[maybe_unused]] inline static void init_logging() {                        \
            logging_enabled =                                                       \
                CHARM_SYCL_NS::logging::scope_enabled(#scope, std::strlen(#scope)); \
        }

#    define DEBUG_FMT(fmtstr, ...)                                                      \
        ({                                                                              \
            if (logging_enabled) {                                                      \
                format::print(std::cout, "[{:13.3f}][{:6s}] " fmtstr "\n",              \
                              CHARM_SYCL_NS::logging::timer_now() / 1e3, logging_scope, \
                              __VA_ARGS__);                                             \
            }                                                                           \
        })

#    define DEBUG_LOG(msg) DEBUG_FMT("{}", (msg))

#    ifndef CHARM_SYCL_ENABLE_LOGGING
#        define CHARM_SYCL_ENABLE_LOGGING
#    endif
#endif
