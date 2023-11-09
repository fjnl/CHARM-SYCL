#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

/// @see 4.6.2 Platform class
struct platform {
    inline platform();

    template <typename DeviceSelector>
    inline explicit platform(const DeviceSelector& deviceSelector);

    inline backend get_backend() const noexcept;

    inline std::vector<device> get_devices(
        info::device_type type = info::device_type::all) const;

    template <typename Param>
    inline typename Param::return_type get_info() const;

    template <typename Param>
    inline typename Param::return_type get_backend_info() const;

    inline bool has(aspect asp) const;

    inline static std::vector<platform> get_platforms();

    inline size_t hash() const;

    inline friend bool operator==(platform const& x, platform const& y) {
        return x.impl_ == y.impl_;
    }

    inline friend bool operator!=(platform const& x, platform const& y) {
        return x.impl_ != y.impl_;
    }

private:
    friend struct runtime::impl_access;

    explicit platform(std::shared_ptr<runtime::platform> const& impl) : impl_(impl) {}

    std::shared_ptr<runtime::platform> impl_;
};

CHARM_SYCL_END_NAMESPACE

namespace std {

template <>
struct hash<CHARM_SYCL_NS::platform> {
    size_t operator()(CHARM_SYCL_NS::platform const& x) const {
        return x.hash();
    }
};

}  // namespace std
