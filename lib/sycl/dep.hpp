#pragma once

#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

namespace rts {

struct device;
struct func_desc;
struct memory_domain;
struct subsystem;

}  // namespace rts

namespace dep {

struct platform;
struct device;
struct event;
struct event_barrier;
struct buffer;
struct dependency_manager;

using rts::memory_domain;

struct id {
    explicit id() : size{0, 0, 0} {}
    explicit id(size_t s1) : size{s1, 0, 0} {}
    explicit id(size_t s1, size_t s2) : size{s1, s2, 0} {}
    explicit id(size_t s1, size_t s2, size_t s3) : size{s1, s2, s3} {}

    size_t size[3];
};

struct range {
    explicit range() : size{1, 1, 1} {}
    explicit range(size_t s1) : size{s1, 1, 1} {}
    explicit range(size_t s1, size_t s2) : size{s1, s2, 1} {}
    explicit range(size_t s1, size_t s2, size_t s3) : size{s1, s2, s3} {}

    size_t size[3];
};

static_assert(std::is_trivially_copyable_v<range>);
static_assert(std::is_trivially_destructible_v<range>);

struct nd_range {
    size_t global[3];
    size_t local[3];
};

static_assert(std::is_trivially_copyable_v<nd_range>);
static_assert(std::is_trivially_destructible_v<nd_range>);

struct accessor {
    size_t size[3];
    size_t offset[3];
};

// Check the memory layout requirements. See also device_accessor.hpp.
static_assert(std::is_trivially_copyable_v<accessor>,
              "Layout must match with the internal struct in the runtime library.");
static_assert(std::is_standard_layout_v<accessor>,
              "Layout must match with the internal struct in the runtime library.");
static_assert(sizeof(accessor) == sizeof(size_t) * 6,
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(accessor, size[0]) == 0,
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(accessor, size[1]) == 1 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(accessor, size[2]) == 2 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(accessor, offset[0]) == 3 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(accessor, offset[1]) == 4 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(offsetof(accessor, offset[2]) == 5 * sizeof(size_t),
              "Layout must match with the internal struct in the runtime library.");
static_assert(std::is_trivially_destructible_v<accessor>);

inline size_t acc_linear_off(accessor const& acc) {
    return acc.offset[2] + acc.offset[1] * acc.size[2] +
           acc.offset[0] * acc.size[1] * acc.size[2];
}

struct local_accessor {
    size_t off;
    size_t size[3];
};

static_assert(std::is_trivially_copyable_v<local_accessor>);
static_assert(std::is_trivially_destructible_v<local_accessor>);

enum class memory_access { none = 0, read_only = 1, write_only = 2, read_write = 3 };

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

    virtual std::shared_ptr<rts::device> const& get() = 0;

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

struct event : runtime::event {};

struct event_barrier : runtime::event_barrier {};

struct task {
    virtual ~task() = default;

    virtual void enable_profiling() = 0;

    virtual void depends_on(event const& ev) = 0;
    virtual void depends_on(std::shared_ptr<task> const& task) = 0;

    void lock() {
        begin_params();
    }

    void unlock() {
        end_params();
    }

    // 1. Select device
    virtual void use_device(device& dev) = 0;
    virtual void use_host() = 0;

    // 2. Set kernel and range
    void set_host_fn() {
        set_host_fn({});
    }
    virtual void set_host_fn(std::function<void()> const& f) = 0;
    virtual void set_kernel(char const* name, uint32_t hash) = 0;

    virtual void set_single() = 0;
    virtual void set_range(range const& range) = 0;
    virtual void set_nd_range(nd_range const& ndr) = 0;

    // Local Memory
    virtual void set_local_mem_size(size_t byte) = 0;

    // Memory operations
    virtual void copy_1d(buffer& src, memory_access src_acc, size_t src_off_byte, buffer& dst,
                         memory_access dst_acc, size_t dst_off_byte, size_t len_byte) = 0;

    virtual void copy_1d(buffer& src, memory_access src_acc, size_t src_off_byte, void* dst,
                         size_t len_byte) = 0;

    virtual void copy_1d(void const* src, buffer& dst, memory_access dst_acc,
                         size_t dst_off_byte, size_t len_byte) = 0;

    virtual void copy_2d(buffer& src, memory_access src_acc, size_t src_off_byte,
                         size_t src_stride, buffer& dst, memory_access dst_acc,
                         size_t dst_off_byte, size_t dst_stride, size_t loop,
                         size_t len_byte) = 0;

    virtual void copy_2d(buffer& src, memory_access src_acc, size_t src_off_byte,
                         size_t src_stride, void* dst, size_t dst_stride, size_t loop,
                         size_t len_byte) = 0;

    virtual void copy_2d(void const* src, size_t src_stride, buffer& dst, memory_access dst_acc,
                         size_t dst_off_byte, size_t dst_stride, size_t loop,
                         size_t len_byte) = 0;

    virtual void copy_3d(buffer& src, memory_access src_acc, size_t src_off_byte,
                         size_t i_src_stride, size_t j_src_stride, buffer& dst,
                         memory_access dst_acc, size_t dst_off_byte, size_t i_dst_stride,
                         size_t j_dst_stride, size_t i_loop, size_t j_loop,
                         size_t len_byte) = 0;

    virtual void copy_3d(buffer& src, memory_access src_acc, size_t src_off_byte,
                         size_t i_src_stride, size_t j_src_stride, void* dst,
                         size_t i_dst_stride, size_t j_dst_stride, size_t i_loop, size_t j_loop,
                         size_t len_byte) = 0;

    virtual void copy_3d(void const* src, size_t i_src_stride, size_t j_src_stride, buffer& dst,
                         memory_access dst_acc, size_t dst_off_byte, size_t i_dst_stride,
                         size_t j_dst_stride, size_t i_loop, size_t j_loop,
                         size_t len_byte) = 0;

    virtual void fill_zero(buffer& src, size_t len_byte) = 0;

    // Function descriptor operation
    virtual void set_desc(rts::func_desc const* desc) = 0;

    // 3. Acquire the global lock
    virtual void begin_params() = 0;

    // 4. Set parameters
    virtual void set_param(void const* ptr, size_t size) = 0;
    virtual void set_buffer_param(buffer& buf, memory_access acc, dep::id const& offset,
                                  size_t offset_byte) = 0;

    // 5. Release the global lock
    virtual void end_params() = 0;

    // 6. Submit to the queue
    virtual std::unique_ptr<event> submit() = 0;
};

struct buffer {
    virtual ~buffer() = default;

    virtual void* get_pointer() = 0;

    virtual void* get_host_pointer() = 0;
};

struct dependency_manager {
    virtual ~dependency_manager() = default;

    virtual std::shared_ptr<task> new_task() = 0;

    virtual std::unique_ptr<buffer> new_buffer(void* h_ptr, size_t element_size,
                                               range size) = 0;

    virtual std::vector<std::shared_ptr<platform>> get_platforms() = 0;
};

std::shared_ptr<dependency_manager> make_dependency_manager(
    std::shared_ptr<rts::subsystem> const& ss);

}  // namespace dep

CHARM_SYCL_END_NAMESPACE
