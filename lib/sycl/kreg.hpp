#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <stdint.h>
#include <charm/sycl/fnv1a.hpp>

namespace kreg {

using CHARM_SYCL_NS::detail::fnv1a;

struct kernel_info {
    kernel_info() = default;

    explicit kernel_info(void* f, int n) : fn(f), is_ndr(n) {}

    void* fn = nullptr;
    int is_ndr = 0;
};

struct kernel_registry {
    virtual ~kernel_registry() = default;

    virtual void add(std::string_view name, uint32_t name_hash, std::string_view kind,
                     uint32_t kind_hash, void* f, int is_ndr) = 0;
    virtual kernel_info const* find(std::string_view name, uint32_t name_hash,
                                    std::string_view kind, uint32_t kind_hash) = 0;
};

kernel_registry& get();

}  // namespace kreg

extern "C" void __s_add_kernel_registry(char const* name, unsigned long name_hash,
                                        char const* kind, unsigned long kind_hash, void* f,
                                        int is_ndr);
