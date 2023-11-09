#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime {

struct handler {
    virtual ~handler() = default;

    virtual void single_task(char const* name, uint32_t hash) = 0;

    virtual void parallel_for(range<3> const& range, char const* name, uint32_t hash) = 0;

    virtual void parallel_for(range<3> const& range, id<3> const& offset, char const* name,
                              uint32_t hash) = 0;

    virtual void parallel_for(nd_range<3> const& ndr, char const* name, uint32_t hash) = 0;

    virtual std::shared_ptr<event> finalize() = 0;

    virtual void begin_binds() = 0;

    virtual void end_binds() = 0;

    virtual void bind(std::shared_ptr<accessor> acc) = 0;
    virtual void bind(std::shared_ptr<local_accessor> const& acc) = 0;
    virtual void bind(void const* addr, size_t size) = 0;

    virtual void copy(std::shared_ptr<accessor> const& src,
                      std::shared_ptr<accessor> const& dest) = 0;

    virtual void copy(std::shared_ptr<accessor> const& src, void* dest) = 0;

    virtual void copy(void const* src, std::shared_ptr<accessor> const& dest) = 0;
};

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
