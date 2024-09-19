#include <algorithm>
#include <type_traits>
#include <strings.h>
#include <utils/target.hpp>

namespace {

std::vector<utils::target> const default_targets_vec = {
    utils::target::CPU_OPENMP, utils::target::NVIDIA_CUDA, utils::target::AMD_HIP};

std::vector<utils::target> const& all_targets_vec = default_targets_vec;

}  // namespace

namespace utils {

char const* show(target t) {
    switch (t) {
        case target::NONE:
            return "none";
        case target::CPU_C:
            return "cpu-c";
        case target::CPU_OPENMP:
            return "cpu-openmp";
        case target::NVIDIA_CUDA:
            return "nvidia-cuda";
        case target::AMD_HIP:
            return "amd-hip";
        default:
            return "UNKNOWN";
    }
}

target from_string(std::string_view str) {
    if (strcasecmp(str.data(), "cpu-c") == 0) {
        return target::CPU_C;
    }
    if (strcasecmp(str.data(), "cpu-openmp") == 0) {
        return target::CPU_OPENMP;
    }
    if (strcasecmp(str.data(), "nvidia-cuda") == 0) {
        return target::NVIDIA_CUDA;
    }
    if (strcasecmp(str.data(), "amd-hip") == 0) {
        return target::AMD_HIP;
    }
    return target::NONE;
}

bool is_valid(target t) {
    switch (t) {
        case target::CPU_C:
        case target::CPU_OPENMP:
        case target::NVIDIA_CUDA:
        case target::AMD_HIP:
            return true;

        default:
            return false;
    }
}

std::vector<target> from_list_string(std::string_view str) {
    auto const* it = str.begin();
    auto const* end = str.end();
    std::vector<target> res;

    while (it < end) {
        auto comma = std::find(it, end, ',');

        auto t = utils::from_string(std::string_view(it, comma - it));
        res.push_back(t);

        if (comma == end) {
            break;
        }
        it = comma + 1;
    }

    return res;
}

std::vector<target> const& all_targets() {
    return all_targets_vec;
}

std::vector<target> const& default_targets() {
    return default_targets_vec;
}

}  // namespace utils
