#pragma once

#include <tuple>
#include <typeindex>
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct property_list {
    virtual ~property_list() = default;

    template <class P>
    P const* get_property() const {
        return reinterpret_cast<P const*>(get_impl(std::type_index(typeid(P))));
    }

protected:
    virtual void const* get_impl(std::type_index const& ti) const = 0;
};

template <class... Ps>
struct property_list_impl final : property_list {
    property_list_impl(Ps&&... props) : data_(std::forward<Ps>(props)...) {}

    void const* get_impl(std::type_index const& ti) const override {
        return get_impl_<0, Ps...>::get(data_, ti);
    }

private:
    template <size_t I, class... Xs>
    struct get_impl_;

    template <size_t I>
    struct get_impl_<I> {
        static void const* get(std::tuple<Ps...> const&, std::type_index const&) {
            return nullptr;
        }
    };

    template <size_t I, class X, class... Xs>
    struct get_impl_<I, X, Xs...> {
        static void const* get(std::tuple<Ps...> const& t, std::type_index const& ti) {
            if (std::type_index(typeid(X)) == ti) {
                return std::addressof(std::get<I>(t));
            }

            return get_impl_<I + 1, Xs...>::get(t, ti);
        }
    };

    std::tuple<Ps...> data_;
};

template <class... Ps>
std::unique_ptr<property_list> make_property_list(Ps&&... props) {
    return std::make_unique<property_list_impl<Ps...>>(std::forward<Ps>(props)...);
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
