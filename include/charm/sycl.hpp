#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

// clang-format off
#include <charm/sycl/config.hpp>
// clang-format on

#include <charm/sycl/access_mode.hpp>
#include <charm/sycl/aspect.hpp>
#include <charm/sycl/backend.hpp>
#include <charm/sycl/device_type.hpp>

// clang-format off
#include <charm/sycl/fwd.hpp>
#include <charm/sycl/runtime/fwd.hpp>
#include <charm/sycl/utils.hpp>
// clang-format on

// clang-format off
#include <charm/sycl/id.hpp>
#include <charm/sycl/range.hpp>
#include <charm/sycl/item.hpp>
#include <charm/sycl/nd_range.hpp>
#include <charm/sycl/nd_item.hpp>
#include <charm/sycl/property.hpp>
#include <charm/sycl/runtime/property.hpp>
// clang-format on

#include <charm/sycl/accessor.hpp>
#include <charm/sycl/buffer.hpp>
#include <charm/sycl/context.hpp>
#include <charm/sycl/device.hpp>
#include <charm/sycl/device_accessor.hpp>
#include <charm/sycl/device_info.hpp>
#include <charm/sycl/event.hpp>
#include <charm/sycl/exception.hpp>
#include <charm/sycl/group.hpp>
#include <charm/sycl/handler.hpp>
#include <charm/sycl/host_accessor.hpp>
#include <charm/sycl/local_accessor.hpp>
#include <charm/sycl/platform.hpp>
#include <charm/sycl/platform_info.hpp>
#include <charm/sycl/queue.hpp>
#include <charm/sycl/selector.hpp>
#include <charm/sycl/vec.hpp>
//
#include <charm/sycl/runtime/accessor.hpp>
#include <charm/sycl/runtime/allocator.hpp>
#include <charm/sycl/runtime/buffer.hpp>
#include <charm/sycl/runtime/context.hpp>
#include <charm/sycl/runtime/device.hpp>
#include <charm/sycl/runtime/event.hpp>
#include <charm/sycl/runtime/handler.hpp>
#include <charm/sycl/runtime/local_accessor.hpp>
#include <charm/sycl/runtime/platform.hpp>
#include <charm/sycl/runtime/queue.hpp>
//
#include <charm/sycl/buffer.ipp>
#include <charm/sycl/context.ipp>
#include <charm/sycl/device.ipp>
#include <charm/sycl/device_accessor.ipp>
#include <charm/sycl/device_info.ipp>
#include <charm/sycl/event.ipp>
#include <charm/sycl/group.ipp>
#include <charm/sycl/handler.ipp>
#include <charm/sycl/host_accessor.ipp>
#include <charm/sycl/id.ipp>
#include <charm/sycl/item.ipp>
#include <charm/sycl/local_accessor.ipp>
#include <charm/sycl/math.ipp>
#include <charm/sycl/platform.ipp>
#include <charm/sycl/platform_info.ipp>
#include <charm/sycl/queue.ipp>
#include <charm/sycl/range.ipp>
#include <charm/sycl/selector.ipp>
#include <charm/sycl/utils.ipp>
//
