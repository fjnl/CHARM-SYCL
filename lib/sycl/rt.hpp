#pragma once
#include <charm/sycl.hpp>
#include "dep.hpp"
#include "rts.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

struct accessor_impl;
struct buffer_impl;
struct device_impl;
struct event_impl;
struct handler_impl;
struct queue_impl;
struct platform_impl;

std::unique_ptr<rts::subsystem> make_dev_rts();
#ifdef HAVE_IRIS
std::unique_ptr<rts::subsystem> make_iris_rts();
#endif
#ifdef HAVE_DEV_RTS_CUDA
std::unique_ptr<rts::subsystem> make_dev_rts_cuda();
#endif
#ifdef HAVE_DEV_RTS_HIP
std::unique_ptr<rts::subsystem> make_dev_rts_hip();
#endif

std::shared_ptr<platform_impl> make_platform(std::shared_ptr<dep::platform>);

std::shared_ptr<device_impl> make_device(std::shared_ptr<runtime::platform> const&,
                                         std::shared_ptr<dep::device>);

std::shared_ptr<buffer_impl> make_buffer(void* init_ptr, std::shared_ptr<void> const& sp,
                                         size_t elemsize, sycl::range<3> const& rng,
                                         sycl::property_list const*);

std::shared_ptr<runtime::event> make_event(std::shared_ptr<dep::event> const&);

std::shared_ptr<accessor_impl> make_accessor(std::shared_ptr<buffer_impl> buf, range<3> range,
                                             id<3> offset, access_mode mode,
                                             property_list const* props);

struct platform_impl final : runtime::platform, std::enable_shared_from_this<platform_impl> {
    platform_impl(std::shared_ptr<dep::platform> p);

    sycl::backend get_backend() const override;

    std::vector<std::shared_ptr<runtime::device>> get_devices() override;

    std::string info_version() const override;

    std::string info_name() const override;

    std::string info_vendor() const override;

private:
    std::shared_ptr<dep::platform> p_;
};

struct buffer_impl final : runtime::buffer {
    explicit buffer_impl(void* init_ptr, std::shared_ptr<void> const& sp, size_t elemsize,
                         sycl::range<3> const& rng, property_list const*);

    void write_back() override;

    void write_back(dep::memory_access);

    sycl::range<3> get_range() const override;

    size_t byte_size() const override;

    void do_writeback(dep::memory_access);

    void set_write_back(bool on) override;

    void set_final_pointer(void* ptr) override;

    void* get_pointer();

    void* get_host_pointer();

    std::shared_ptr<dep::buffer> to_lower();

    size_t elem_size() const;

private:
    size_t elemsize_;
    sycl::range<3> range_;
    bool write_back_;
    void* write_back_ptr_;
    std::shared_ptr<void> sp_;
    std::shared_ptr<dep::buffer> dep_;
};

struct device_impl final : runtime::device, std::enable_shared_from_this<device_impl> {
    explicit device_impl(std::shared_ptr<runtime::platform> const& plt,
                         std::shared_ptr<dep::device> dev);

    sycl::backend get_backend() const override;

    std::shared_ptr<runtime::platform> const& get_platform() override;

    sycl::aspect get_aspect() const override;

    sycl::info::device_type info_device_type() const override;

    std::string info_name() const override;

    std::string info_vendor() const override;

    std::string info_driver_version() const override;

    std::shared_ptr<dep::device> to_lower();

private:
    std::shared_ptr<runtime::platform> plt_;
    std::shared_ptr<dep::device> dev_;
};

struct queue_impl final : runtime::queue, std::enable_shared_from_this<queue_impl> {
    explicit queue_impl(std::shared_ptr<runtime::context> ctx,
                        std::shared_ptr<runtime::device> dev, property_list const* props);

    sycl::backend get_backend() const noexcept override;

    std::shared_ptr<runtime::device> get_device() const override;

    std::shared_ptr<runtime::context> get_context() const override;

    void add(std::shared_ptr<runtime::event> const&) override;

    void wait() override;

    bool profiling_enabled() const;

private:
    std::shared_ptr<runtime::context> ctx_;
    std::shared_ptr<runtime::device> dev_;
    std::vector<std::shared_ptr<runtime::event>> events_;
    bool profiling_enabled_;
};

struct handler_impl final : runtime::handler, std::enable_shared_from_this<handler_impl> {
    explicit handler_impl(queue_impl& q);

    void single_task(char const* name, uint32_t hash) override;

    void parallel_for(sycl::range<3> const& range, char const* name, uint32_t hash) override;

    void parallel_for(sycl::range<3> const& range, sycl::id<3> const& offset, char const* name,
                      uint32_t hash) override;

    void parallel_for(sycl::nd_range<3> const& ndr, char const* name, uint32_t hash) override;

    std::shared_ptr<runtime::event> finalize() override;

    void begin_binds() override;

    void end_binds() override;

    void bind(std::shared_ptr<runtime::accessor> acc) override;

    void bind(std::shared_ptr<runtime::local_accessor> const& acc) override;

    void bind(void const* ptr, size_t size) override;

    void copy(std::shared_ptr<accessor> const& src,
              std::shared_ptr<accessor> const& dest) override;

    void copy(std::shared_ptr<accessor> const& src, void* dest) override;

    void copy(void const* src, std::shared_ptr<accessor> const& dest) override;

    void lock() {
        begin_binds();
    }

    void unlock() {
        end_binds();
    }

    size_t alloc_smem(size_t byte, size_t align, bool is_array);

private:
    queue_impl& q_;
    std::shared_ptr<dep::task> task_;
    size_t lmem_;
};

struct accessor_impl final : runtime::accessor {
    explicit accessor_impl(std::shared_ptr<handler_impl> const& handler,
                           std::shared_ptr<buffer_impl> const& buf, range<3> range,
                           id<3> offset, access_mode mode, property_list const* props);

    range<3> get_range() const override;

    id<3> get_offset() const override;

    void* get_pointer() override;

    std::shared_ptr<runtime::buffer> get_buffer() override;

    std::shared_ptr<buffer_impl> get();

    access_mode get_access_mode() const;

private:
    range<3> range_;
    id<3> offset_;
    std::shared_ptr<handler_impl> handler_;
    std::shared_ptr<buffer_impl> buffer_;
    access_mode mode_;
};

struct local_accessor_impl final : runtime::local_accessor {
    explicit local_accessor_impl(std::shared_ptr<handler_impl> const& handler, int dim,
                                 size_t elem_size, size_t align, range<3> range,
                                 property_list const* props);

    size_t get_offset() const;

private:
    size_t off_;
};

struct host_accessor_impl final : runtime::host_accessor {
    explicit host_accessor_impl(std::shared_ptr<buffer_impl> const& buf, range<3> range,
                                id<3> offset, access_mode mode, property_list const* props);

    range<3> get_range() const override;

    id<3> get_offset() const override;

    void* get_pointer() override;

    std::shared_ptr<runtime::buffer> get_buffer() override;

    std::shared_ptr<buffer_impl> get();

    access_mode get_access_mode() const;

private:
    range<3> range_;
    id<3> offset_;
    std::shared_ptr<buffer_impl> buffer_;
    access_mode mode_;
};

struct global_state {
    static std::shared_ptr<dep::dependency_manager> get_depmgr() {
        static std::mutex lock;
        static std::weak_ptr<dep::dependency_manager> mgr_weak;

        std::scoped_lock lk(lock);

        if (auto mgr = mgr_weak.lock()) {
            return mgr;
        }

        auto mgr = sycl::dep::make_dependency_manager(rts::make_subsystem());
        mgr_weak = mgr;
        return mgr;
    }
};

inline rts::range convert(sycl::range<3> const& r) {
    return rts::range(r[0], r[1], r[2]);
}

inline rts::id convert(sycl::id<3> const& i) {
    return rts::id(i[0], i[1], i[2]);
}

inline rts::nd_range convert(sycl::nd_range<3> const& r) {
    rts::nd_range ndr;
    ndr.global[0] = r.get_global_range()[0];
    ndr.global[1] = r.get_global_range()[1];
    ndr.global[2] = r.get_global_range()[2];
    ndr.local[0] = r.get_local_range()[0];
    ndr.local[1] = r.get_local_range()[1];
    ndr.local[2] = r.get_local_range()[2];
    return ndr;
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
