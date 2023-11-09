#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct queue {
    virtual ~queue() = default;

    virtual backend get_backend() const noexcept = 0;

    virtual std::shared_ptr<device> get_device() const = 0;

    virtual std::shared_ptr<context> get_context() const = 0;

    virtual void add(std::shared_ptr<runtime::event> const&) = 0;

    virtual void wait() = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
