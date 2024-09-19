#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

/// @see 4.6.4. Device class
struct device {
    inline device();

    template <typename DeviceSelector>
    explicit inline device(const DeviceSelector& deviceSelector);

    inline backend get_backend() const noexcept;

    inline bool is_cpu() const;

    inline bool is_gpu() const;

    inline bool is_accelerator() const;

    inline platform get_platform() const;

    template <typename Param>
    inline typename Param::return_type get_info() const;

    template <typename Param>
    inline typename Param::return_type get_backend_info() const;

    inline bool has(aspect asp) const;

    static inline std::vector<device> get_devices(
        info::device_type deviceType = info::device_type::all);

    size_t hash() const;

private:
    friend struct runtime::impl_access;

    explicit device(runtime::device_ptr const& impl) : impl_(impl) {}

    runtime::device_ptr impl_;
};

CHARM_SYCL_END_NAMESPACE

namespace std {

template <>
struct hash<CHARM_SYCL_NS::device> {
    size_t operator()(CHARM_SYCL_NS::device const& x) const {
        return x.hash();
    }
};

}  // namespace std
