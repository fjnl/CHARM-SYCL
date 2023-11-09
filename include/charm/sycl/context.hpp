#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

struct context {
    inline explicit context(const property_list& propList = {});

    inline explicit context(async_handler asyncHandler, const property_list& propList = {});

    inline explicit context(const device& dev, const property_list& propList = {});

    inline explicit context(const device& dev, async_handler asyncHandler,
                            const property_list& propList = {});

    inline explicit context(const std::vector<device>& deviceList,
                            const property_list& propList = {});

    inline explicit context(const std::vector<device>& deviceList, async_handler asyncHandler,
                            const property_list& propList = {});

    inline backend get_backend() const noexcept;

    inline platform get_platform() const;

    inline std::vector<device> get_devices() const;

    template <typename Param>
    inline typename Param::return_type get_info() const;

    template <typename Param>
    inline typename Param::return_type get_backend_info() const;

private:
    friend struct runtime::impl_access;

    explicit context(std::shared_ptr<runtime::context> const& impl) : impl_(impl) {}

    std::shared_ptr<runtime::context> impl_;
};

namespace detail {

inline context get_default_context(platform plt);

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
