#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <stdint.h>
#include <charm/sycl/config.hpp>
#include "dep.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace rts {

struct subsystem;
struct platform;
struct device;
struct memory_domain;
struct buffer;
struct task;
struct event;

using dep::accessor;
using dep::id;
using dep::memory_access;
using dep::nd_range;
using dep::range;

using memory_handle = uintptr_t;
using dom_id = uint_fast32_t;

static constexpr auto INVALID_DOM_ID = static_cast<dom_id>(-1);
static constexpr auto HOST_DOM_ID = static_cast<dom_id>(0);

struct subsystem {
    virtual ~subsystem() = default;

    virtual std::vector<std::shared_ptr<platform>> get_platforms() const = 0;

    virtual std::shared_ptr<task> new_task() = 0;

    virtual std::unique_ptr<buffer> new_buffer(void* h_ptr, size_t element_size,
                                               range const& size) = 0;

    virtual memory_domain& get_host_memory_domain() = 0;

    virtual void shutdown() = 0;
};

struct platform {
    virtual ~platform() = default;

    virtual std::vector<std::shared_ptr<device>> get_devices() const = 0;

    virtual std::string info_name() const = 0;
    virtual std::string info_vendor() const = 0;
    virtual std::string info_version() const = 0;
};

struct device {
    virtual ~device() = default;

    virtual memory_domain const& get_memory_domain() const = 0;

    virtual bool is_cpu() const = 0;
    virtual bool is_gpu() const = 0;
    virtual bool is_accelerator() const = 0;
    virtual bool is_fpga() const = 0;
    virtual bool is_custom() const = 0;
    virtual bool is_host() const = 0;

    virtual std::string info_name() const = 0;
    virtual std::string info_vendor() const = 0;
    virtual std::string info_driver_version() const = 0;
};

struct memory_domain {
    virtual ~memory_domain() = default;

    inline bool is_host() const {
        return id() == 0;
    }

    virtual dom_id id() const = 0;
};

struct host_memory_domain final : memory_domain {
    dom_id id() const override {
        return 0;
    }
};

struct buffer {
    virtual ~buffer() = default;

    virtual void* get_pointer() = 0;

    virtual void* get_host_pointer() = 0;

    virtual range const& get_size() const = 0;
};

struct task {
    virtual ~task() = default;

    virtual void enable_profiling() = 0;

    virtual void depends_on(std::shared_ptr<task> const& task) = 0;

    // 1.a. Select Device
    virtual void use_device() = 0;
    virtual void set_device(device& dev) = 0;

    // 1.b Select Host
    virtual void use_host() = 0;

    // 2.a. Select Device Kernel
    virtual void set_kernel(char const* name, uint32_t hash) = 0;

    // 2.b. Set Host Functor
    virtual void set_host(std::function<void()> const& f) = 0;

    // 2.c Memory Operations
    virtual void copy_1d(buffer& src, size_t src_off_byte, buffer& dst, size_t dst_off_byte,
                         size_t len_byte) = 0;

    virtual void copy_1d(buffer& src, size_t src_off_byte, void* dst, size_t len_byte) = 0;

    virtual void copy_1d(void const* src, buffer& dst, size_t dst_off_byte,
                         size_t len_byte) = 0;

    virtual void copy_2d(buffer& src, size_t src_off_byte, size_t src_stride, buffer& dst,
                         size_t dst_off_byte, size_t dst_stride, size_t loop,
                         size_t len_byte) = 0;

    virtual void copy_2d(buffer& src, size_t src_off_byte, size_t src_stride, void* dst,
                         size_t dst_stride, size_t loop, size_t len_byte) = 0;

    virtual void copy_2d(void const* src, size_t src_stride, buffer& dst, size_t dst_off_byte,
                         size_t dst_stride, size_t loop, size_t len_byte) = 0;

    virtual void copy_3d(buffer& src, size_t src_off_byte, size_t i_src_stride,
                         size_t j_src_stride, buffer& dst, size_t dst_off_byte,
                         size_t i_dst_stride, size_t j_dst_stride, size_t i_loop, size_t j_loop,
                         size_t len_byte) = 0;

    virtual void copy_3d(buffer& src, size_t src_off_byte, size_t i_src_stride,
                         size_t j_src_stride, void* dst, size_t i_dst_stride,
                         size_t j_dst_stride, size_t i_loop, size_t j_loop,
                         size_t len_byte) = 0;

    virtual void copy_3d(void const* src, size_t i_src_stride, size_t j_src_stride, buffer& dst,
                         size_t dst_off_byte, size_t i_dst_stride, size_t j_dst_stride,
                         size_t i_loop, size_t j_loop, size_t len_byte) = 0;

    // 3. Set parallelism
    virtual void set_single() = 0;
    virtual void set_range(range const& range) = 0;
    virtual void set_range(range const& range, id const& offset) = 0;
    virtual void set_nd_range(nd_range const& ndr) = 0;

    // Local Memory
    virtual void set_local_mem_size(size_t byte) = 0;

    // 4. Set Parameters
    virtual void set_param(void const* ptr, size_t size) = 0;
    virtual void set_buffer_param(buffer& buf, void* h_ptr, memory_domain const& dom,
                                  memory_access acc, rts::id const& offset,
                                  size_t offset_byte) = 0;

    // 5. Submit this task
    virtual std::unique_ptr<event> submit() = 0;
};

struct event : dep::event {};

struct event_barrier : dep::event_barrier {};

std::unique_ptr<subsystem> make_subsystem();

}  // namespace rts

CHARM_SYCL_END_NAMESPACE
