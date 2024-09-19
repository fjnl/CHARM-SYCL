#include "logging.hpp"

#if defined(NDEBUG) && !defined(CHARM_SYCL_ENABLE_LOGGING)
// nothing
#else
#    include <algorithm>
#    include <chrono>

namespace {

using clock_type = std::chrono::high_resolution_clock;

clock_type::time_point t_start;

char const* getenv() {
    return std::getenv("CHARM_SYCL_DEBUG");
}

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE
namespace logging {

bool global_enabled() {
    return getenv();
}

bool scope_enabled(char const* name, size_t name_len) {
    auto const* str = getenv();
    if (!str) {
        return false;
    }

    auto const* it = str;
    auto const* end = str + strlen(str);

    while (it < end) {
        auto comma = std::find(it, end, ',');
        if (static_cast<size_t>(comma - it) == name_len && std::equal(it, comma, name)) {
            return true;
        }
        it = comma + 1;
    }
    return false;
}

uint64_t timer_now() {
    auto const t = clock_type::now();
    auto const dt = std::chrono::duration_cast<std::chrono::nanoseconds>(t - t_start);
    return dt.count();
}

void timer_reset() {
    t_start = clock_type::now();
}

}  // namespace logging
CHARM_SYCL_END_NAMESPACE
#endif

#include <mutex>
#include <thread>

CHARM_SYCL_BEGIN_NAMESPACE
namespace logging {

bool info_enable = false;
bool warn_enable = false;
static std::mutex log_mutex;

bool parse_to_bool(char const* v, bool default_value) {
    if (!v) {
        return default_value;
    }
    return strcasecmp(v, "1") == 0 || strcasecmp(v, "y") == 0 || strcasecmp(v, "yes") == 0 ||
           strcasecmp(v, "t") == 0 || strcasecmp(v, "true") == 0;
}

void g_init_logging() {
    static std::once_flag initialized;
    std::call_once(initialized, [] {
        info_enable = parse_to_bool(getenv("CHARM_SYCL_INFO"), true);
        warn_enable = parse_to_bool(getenv("CHARM_SYCL_WARN"), true);
    });
}

void info(std::string_view msg) {
    if (info_enable) {
        std::scoped_lock lk(log_mutex);
        format::print(std::cerr, "INFO: {}\n", msg);
    }
}

void warn(std::string_view msg) {
    if (warn_enable) {
        std::scoped_lock lk(log_mutex);
        format::print(std::cerr, "WARN: {}\n", msg);
    }
}

void fatal(std::string_view msg) {
    {
        std::scoped_lock lk(log_mutex);
        format::print(std::cerr, "FATAL ERROR: {}\n", msg);
    }
    std::abort();
}

}  // namespace logging
CHARM_SYCL_END_NAMESPACE
