#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace detail {

struct selector_result {
    std::optional<device> dev;
    int score = -1;

    template <class Selector>
    void merge(Selector const& sel, std::vector<device> const& dev_list) {
        for (auto& d : dev_list) {
            auto const dev_score = sel(d);
            if (score < dev_score) {
                score = dev_score;
                dev = d;
            }
        }
    }
};

template <class Selector>
inline device select_device(Selector const& selector) {
    detail::selector_result res;
    for (auto p : platform::get_platforms()) {
        res.merge(selector, p.get_devices());
    }

    if (!res.dev) {
        throw_error(errc::platform, "no such device");
    }
    return *res.dev;
}

}  // namespace detail

CHARM_SYCL_END_NAMESPACE
