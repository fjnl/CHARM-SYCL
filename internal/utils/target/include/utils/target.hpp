#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

namespace utils {

enum class target : uint32_t {
    NONE = UINT32_C(0),
    CPU_C = (UINT32_C(1) << 0),
    CPU_OPENMP = (UINT32_C(1) << 1),
    NVIDIA_CUDA = (UINT32_C(1) << 2),
    AMD_HIP = (UINT32_C(1) << 3),
};

inline bool is_cpu(target t) {
    return t == target::CPU_C || t == target::CPU_OPENMP;
}

inline bool is_openmp(target t) {
    return t == target::CPU_OPENMP;
}

inline bool is_cuda(target t) {
    return t == target::NVIDIA_CUDA;
}

inline bool is_hip(target t) {
    return t == target::AMD_HIP;
}

inline bool is_cxx(target t) {
    return is_cuda(t) || is_hip(t);
}

char const* show(target t);

target from_string(std::string_view str);

std::vector<target> from_list_string(std::string_view str);

bool is_valid(target t);

std::vector<target> const& all_targets();

std::vector<target> const& default_targets();

}  // namespace utils
