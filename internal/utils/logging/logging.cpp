#if defined(NDEBUG) && !defined(CHARM_SYCL_ENABLE_LOGGING)
// nothing
#else
#    include <algorithm>
#    include <chrono>
#    include <utils/logging.hpp>

namespace {

using clock_type = std::chrono::high_resolution_clock;

clock_type::time_point t_start;

char const* getenv() {
    return std::getenv("CHARM_SYCL_DEBUG");
}

}  // namespace

namespace utils::logging {

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

}  // namespace utils::logging
#endif
