#include <array>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <assert.h>
#include <iris/iris_interface.hpp>
#include <unistd.h>
#include "../fiber.hpp"
#include "../format.hpp"
#include "../interfaces.hpp"
#include "../kreg.hpp"
#include "../logging.hpp"
#include "../rts.hpp"

namespace {

LOGGING_DEFINE_SCOPE(iris)

bool USE_IRIS_DEPEND = false;
bool USE_IRIS_DATA = false;
bool USE_IRIS_RANDOM = false;
bool USE_IRIS_FTF = false;
bool USE_IRIS_SDQ = false;
bool USE_IRIS_ROUNDROBIN = false;
bool USE_IRIS_CPU = false;
bool USE_IRIS_GPU = false;

namespace rts = CHARM_SYCL_NS::rts;

[[noreturn]] void throw_errno_impl(char const* errmsg, int errno_,
                                   char const* file = __builtin_FILE(),
                                   int line = __builtin_LINE()) {
    auto const what = format::format("Errot at {}:{}: {}", file, line, errmsg);
    throw std::system_error(std::error_code(errno_, std::generic_category()), what);
}

#define throw_errno(errmsg)                  \
    ({                                       \
        /* save errno immediately */         \
        auto const errno__ = errno;          \
        throw_errno_impl((errmsg), errno__); \
    })

struct parallel_params {
    std::array<size_t, 3> gws;
    std::array<size_t, 3> lws;
    bool is_ndr = false;
};

struct memory_domain_impl : rts::memory_domain {
    rts::dom_id id() const override {
        return 1;
    }
};

static memory_domain_impl iris_dom;

template <class IRIS>
struct device_impl : rts::device {
    explicit device_impl(int policy) : policy_(policy) {
        init();
    }

    device_impl(device_impl const&) = delete;

    device_impl(device_impl&&) = delete;

    device_impl& operator=(device_impl const&) = delete;

    device_impl& operator=(device_impl&&) = delete;

    bool is_cpu() const override {
        return types_ & IRIS::cpu;
    }

    bool is_gpu() const override {
        return types_ & IRIS::gpu;
    }

    bool is_accelerator() const override {
        return types_ & (IRIS::gpu | IRIS::fpga);
    }

    bool is_fpga() const override {
        return types_ & IRIS::fpga;
    }

    bool is_custom() const override {
        return false;
    }

    bool is_host() const override {
        return false;
    }

    rts::memory_domain& get_memory_domain() const override {
        return iris_dom;
    }

    std::string info_name() const override {
        char info[256];
        std::string type = "IRIS Unknown Device";
        size_t size;
        IRIS::iris_device_info(0, IRIS::name, &info, &size);
        switch (policy_) {
            case IRIS::cpu:
                type = "IRIS CPU Device";
                break;
            case IRIS::gpu:
                type = "IRIS GPU Device";
                break;
            case IRIS::fpga:
                type = "IRIS FPGA Device";
                break;
            case IRIS::sdq:
                type = "IRIS Shortest Device Queue";
                break;
            case IRIS::ftf:
                type = "IRIS Device First-to-Finish";
                break;
            case IRIS::depend:
                type = "IRIS Device Depend";
                break;
            case IRIS::data:
                type = "IRIS Device Data";
                break;
            case IRIS::random:
                type = "IRIS Device Random";
                break;
            case IRIS::roundrobin:
                type = "IRIS Roundrobin Device";
                break;
            default:
                type = "IRIS Unknown Device";
                break;
        }

#ifndef CHARM_SYCL_USE_IRIS_DMEM
        return std::string(info) + " (" + type + ")";
#else
        return std::string(info) + " (" + type + ") [+dmem]";
#endif
    }

    std::string info_vendor() const override {
        char vendor[256];
        size_t size;
        IRIS::iris_device_info(0, IRIS::vendor, vendor, &size);
        return std::string(vendor);
    }

    std::string info_driver_version() const override {
        return "none";
    }

    bool available() const {
        return ndev_ > 0;
    }

    int policy() const {
        return policy_;
    }

private:
    static bool is_wildcard(int type) {
        return type & (IRIS::sdq | IRIS::ftf | IRIS::depend | IRIS::roundrobin | IRIS::random |
                       IRIS::data);
    }

    void init() {
        int n = 0;
        if (IRIS::iris_device_count(&n) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_device_count() failed");
        }

        for (int i = 0; i < n; i++) {
            int type = -1;

            if (IRIS::iris_device_info(i, IRIS::type, &type, nullptr) != IRIS::SUCCESS) {
                throw std::runtime_error("iris_device_info() failed");
            }
            types_ |= type;

            if (is_wildcard(policy_) || (type & policy_)) {
                ndev_ += 1;
            }
        }
    }

    int policy_;
    int ndev_ = 0;
    unsigned int types_ = 0;
};

template <class IRIS>
struct platform_impl : rts::platform {
    platform_impl() {
        check_env();

        std::vector<unsigned int> policies;

        if (USE_IRIS_FTF) {
            policies.push_back(IRIS::ftf);
        }
        if (USE_IRIS_SDQ) {
            policies.push_back(IRIS::sdq);
        }
        if (USE_IRIS_DEPEND) {
            policies.push_back(IRIS::depend);
        }
        if (USE_IRIS_DATA) {
            policies.push_back(IRIS::data);
        }
        if (USE_IRIS_RANDOM) {
            policies.push_back(IRIS::random);
        }
        if (USE_IRIS_ROUNDROBIN) {
            policies.push_back(IRIS::roundrobin);
        }
        if (USE_IRIS_GPU) {
            policies.push_back(IRIS::gpu);
        }
        if (USE_IRIS_CPU) {
            policies.push_back(IRIS::cpu);
        }

        if (policies.empty()) {
            policies.push_back(IRIS::gpu);
            policies.push_back(IRIS::cpu);
            // policies.push_back(iris_fpga);
        }

        for (auto const& policy : policies) {
            if (auto d = std::make_shared<device_impl<IRIS>>(policy); d->available()) {
                devs_.push_back(d);
            }
        }
    }

    platform_impl(platform_impl const&) = delete;

    platform_impl(platform_impl&&) = delete;

    platform_impl& operator=(platform_impl const&) = delete;

    platform_impl& operator=(platform_impl&&) = delete;

    std::vector<std::shared_ptr<rts::device>> get_devices() const override {
        return devs_;
    };

    std::string info_name() const override {
        char name[256];
        size_t size;
        IRIS::iris_platform_info(0, IRIS::name, name, &size);
        return "IRIS Platform (" + std::string(name) + ")";
    }

    std::string info_vendor() const override {
        return "IRIS";
    }

    std::string info_version() const override {
        return "0.0.1";
    }

private:
    void check_env() {
        if (auto const* policy = ::getenv("CHARM_SYCL_IRIS_POLICY")) {
            USE_IRIS_FTF = false;
            USE_IRIS_SDQ = false;
            USE_IRIS_ROUNDROBIN = false;
            USE_IRIS_DEPEND = false;
            USE_IRIS_DATA = false;
            USE_IRIS_RANDOM = false;
            USE_IRIS_CPU = false;
            USE_IRIS_GPU = false;

            if (strcasecmp(policy, "sdq") == 0) {
                USE_IRIS_SDQ = true;
            } else if (strcasecmp(policy, "ftf") == 0) {
                USE_IRIS_FTF = true;
            } else if (strcasecmp(policy, "depend") == 0) {
                USE_IRIS_DEPEND = true;
            } else if (strcasecmp(policy, "data") == 0) {
                USE_IRIS_DATA = true;
            } else if (strcasecmp(policy, "random") == 0) {
                USE_IRIS_RANDOM = true;
            } else if (strcasecmp(policy, "roundrobin") == 0) {
                USE_IRIS_ROUNDROBIN = true;
            } else if (strcasecmp(policy, "cpu") == 0) {
                USE_IRIS_CPU = true;
            } else if (strcasecmp(policy, "gpu") == 0) {
                USE_IRIS_GPU = true;
            } else {
                throw std::runtime_error(
                    format::format("Unknown CHARM_SYCL_IRIS_POLICY value: {}", policy));
            }
        }
    }

    std::vector<std::shared_ptr<rts::device>> devs_;
};

template <class IRIS>
struct buffer_impl final : rts::buffer {
    using mem_t = typename IRIS::mem_t;

    explicit buffer_impl(void* h_ptr, size_t element_size, rts::range const& size)
        : element_size_(element_size), size_(size) {
#ifndef CHARM_SYCL_USE_IRIS_DMEM
        (void)h_ptr;
        if (IRIS::iris_mem_create(size_byte(), &mem_) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_mem_create() failed");
        }
#else
        if (IRIS::iris_data_mem_create(&mem_, h_ptr, size_byte()) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_data_mem_create() failed");
        }
#endif
    }

    buffer_impl(buffer_impl const&) = delete;

    buffer_impl(buffer_impl&&) = delete;

    buffer_impl& operator=(buffer_impl const&) = delete;

    buffer_impl& operator=(buffer_impl&&) = delete;

    ~buffer_impl() {
        IRIS::iris_mem_release(mem_);
    }

    void* get_pointer() override {
        return &mem_;
    }

    void* get_host_pointer() override {
        return h_ptr_;
    }

    mem_t& get() {
        return mem_;
    }

    rts::range const& get_size() const override {
        return size_;
    }

    rts::range offset() const {
        return rts::range(0, 0, 0);
    }

    size_t size_byte() const {
        return element_size_ * size_.size[0] * size_.size[1] * size_.size[2];
    }

    size_t offset_byte() const {
        return 0;
    }

private:
    mem_t mem_;
    size_t element_size_;
    rts::range size_;
    void* h_ptr_ = nullptr;
};

template <class IRIS>
struct event_barrier_impl final : rts::event_barrier {
    void wait() override {
        if (need_sync_) {
            if (IRIS::iris_synchronize() != IRIS::SUCCESS) {
                throw std::runtime_error("iris_synchronize() failed");
            }
        }
    }

    void add(sycl::runtime::event& ev) override;

private:
    bool need_sync_ = false;
};

template <class IRIS>
struct event_impl final : rts::event {
    using task_t = typename IRIS::task_t;

    explicit event_impl(task_t const& task, bool empty) : task_(task), empty_(empty) {}

    ~event_impl() {}

    event_impl(event_impl const&) = delete;

    event_impl(event_impl&&) = delete;

    event_impl& operator=(event_impl const&) = delete;

    event_impl& operator=(event_impl&&) = delete;

    sycl::runtime::event_barrier* create_barrier() override {
        return new event_barrier_impl<IRIS>();
    }

    void release_barrier(sycl::runtime::event_barrier* ptr) override {
        delete ptr;
    }

    bool is_empty() const {
        return empty_;
    }

    uint64_t profiling_command_submit() override {
        IRIS::iris_synchronize();

        uint64_t submission_start_time;
        IRIS::iris_task_info(task_, IRIS::task_time_submit, &submission_start_time, nullptr);
        return submission_start_time;
    }

    uint64_t profiling_command_start() override {
        IRIS::iris_synchronize();

        size_t kernel_start_time;
        IRIS::iris_task_info(task_, IRIS::task_time_start, &kernel_start_time, nullptr);
        return kernel_start_time;
    }

    uint64_t profiling_command_end() override {
        IRIS::iris_synchronize();

        size_t kernel_end_time;
        IRIS::iris_task_info(task_, IRIS::task_time_end, &kernel_end_time, nullptr);
        return kernel_end_time;
    }

    auto get() const {
        return task_;
    }

private:
    task_t task_;
    bool empty_;
};

template <class IRIS>
void event_barrier_impl<IRIS>::add(sycl::runtime::event& ev) {
    if (!static_cast<event_impl<IRIS>&>(ev).is_empty()) {
        need_sync_ = true;
    }
}

template <class IRIS>
struct task_impl final : rts::task, std::enable_shared_from_this<task_impl<IRIS>> {
    using task_t = typename IRIS::task_t;
    using kernel_t = typename IRIS::kernel_t;

    explicit task_impl() {
        task_.emplace();
        if (IRIS::iris_task_create(&*task_) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_task_create() failed");
        }
    }

    ~task_impl() {
        // TODO: examine!
        // if (profiling_enabled) {
        //     iris_task_release(*task_);
        // }
    }

    task_impl(task_impl const&) = delete;

    task_impl(task_impl&&) = delete;

    task_impl& operator=(task_impl const&) = delete;

    task_impl& operator=(task_impl&&) = delete;

    /* ----------- */

    void enable_profiling() override {
        profiling_enabled = true;
        IRIS::iris_task_retain(*task_, true);
    }

    void depends_on(rts::event const& ev) override {
        auto task = static_cast<event_impl<IRIS> const&>(ev).get();

        if (IRIS::iris_task_depend(*task_, 1, &task) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_task_depend() failed");
        }
    }

    void depends_on(std::shared_ptr<rts::task> const& dep) override {
        DEBUG_FMT("depends_on(this={}, task={}) {} {}", format::ptr(this),
                  format::ptr(dep.get()), format::ptr(&*task_),
                  format::ptr(&*std::static_pointer_cast<task_impl<IRIS>>(dep)->task_));

        if (IRIS::iris_task_depend(*task_, 1,
                                   &*std::static_pointer_cast<task_impl<IRIS>>(dep)->task_) !=
            IRIS::SUCCESS) {
            throw std::runtime_error("iris_task_depend() failed");
        }
    }

    /* ----------- */

    void use_device() override {
        is_host_ = false;
    }

    void set_device(rts::device& dev) override {
        policy_ = static_cast<device_impl<IRIS>&>(dev).policy();
    }

    /* ----------- */

    void use_host() override {
        is_host_ = true;
        policy_ = IRIS::cpu;
    }

    /* ----------- */

    void set_kernel(char const* name, uint32_t) override {
        kernel_.emplace();
        if (IRIS::iris_kernel_create(name, &*kernel_) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_kernel_create() failed");
        }
        empty_ = false;
    }

    /* ----------- */

    void set_host(std::function<void()> const& f) override {
        assert(is_host_ == true);
        hostfn_ = f;
        if (hostfn_) {
            empty_ = false;
        }
    }

    /* ----------- */

    void set_single() override {
        par_.gws[0] = 1;
        par_.gws[1] = 1;
        par_.gws[2] = 1;

        par_.lws[0] = 1;
        par_.lws[1] = 1;
        par_.lws[2] = 1;
    }

    void set_range(rts::range const& r) override {
        par_.gws[0] = ((r.size[2] + 255) / 256) * 256;
        par_.gws[1] = r.size[1];
        par_.gws[2] = r.size[0];

        par_.lws[0] = 256;
        par_.lws[1] = 1;
        par_.lws[2] = 1;
    }

    void set_nd_range(rts::nd_range const& ndr) override {
        par_.gws[0] = ndr.global[2];
        par_.gws[1] = ndr.global[1];
        par_.gws[2] = ndr.global[0];

        par_.lws[0] = ndr.local[2];
        par_.lws[1] = ndr.local[1];
        par_.lws[2] = ndr.local[0];
    }

    void set_local_mem_size(size_t byte) override {
        if (byte > 0) {
            // TODO
            fprintf(stderr, "Error: Local memory is not supported on IRIS RTS\n");
            std::abort();
        }
    }

    /* ----------- */

    void set_param(void const* ptr, size_t size) override {
        if (kernel_) {
            if (IRIS::iris_kernel_setarg(*kernel_, arg_idx_, size, const_cast<void*>(ptr)) !=
                IRIS::SUCCESS) {
                throw std::runtime_error("iris_kernel_setarg() failed");
            }
        }

        arg_idx_++;
    }

    void set_buffer_param(rts::buffer& buf, void* h_ptr, rts::memory_domain const& dom,
                          rts::memory_access ma, rts::id const& offset,
                          size_t offset_byte) override {
        auto& buf_ = static_cast<buffer_impl<IRIS>&>(buf);
        auto const htod = (ma != rts::memory_access::write_only) && !is_host_ && dom.is_host();
        auto const dtoh = (ma != rts::memory_access::write_only) && is_host_ && !dom.is_host();

        (void)offset;
        DEBUG_FMT(
            "set_buffer_param(h_ptr={}, dom={}, ma={}, off=[{}, {}, {}], off_byte={}) htod={} "
            "dtoh={}",
            format::ptr(h_ptr), dom.id(), static_cast<int>(ma), offset.size[0], offset.size[1],
            offset.size[2], offset_byte, htod, dtoh);

#ifdef CHARM_SYCL_USE_IRIS_DMEM
        (void)h_ptr;
#endif

        if (htod) {
#ifndef CHARM_SYCL_USE_IRIS_DMEM
            if (IRIS::iris_task_h2d(*task_, buf_.get(), buf_.offset_byte(), buf_.size_byte(),
                                    h_ptr) != IRIS::SUCCESS) {
                throw std::runtime_error("iris_task_h2d() failed");
            }
#endif
            empty_ = false;
        } else if (dtoh) {
#ifndef CHARM_SYCL_USE_IRIS_DMEM
            if (IRIS::iris_task_d2h(*task_, buf_.get(), buf_.offset_byte(), buf_.size_byte(),
                                    h_ptr) != IRIS::SUCCESS) {
                throw std::runtime_error("iris_task_d2h() failed");
            }
#else
            if (IRIS::iris_task_dmem_flush_out(*task_, buf_.get()) != IRIS::SUCCESS) {
                throw std::runtime_error("iris_task_dmem_flush_out() failed");
            }
#endif
            empty_ = false;
        }

        if (kernel_) {
            size_t mode = 0;

            switch (ma) {
                case rts::memory_access::read_only:
                    mode = IRIS::r;
                    break;

                case rts::memory_access::write_only:
                    mode = IRIS::w;
                    break;

                default:
                    mode = IRIS::rw;
                    break;
            }

            if (IRIS::iris_kernel_setmem_off(*kernel_, arg_idx_, buf_.get(), offset_byte,
                                             mode) != IRIS::SUCCESS) {
                throw std::runtime_error("iris_kernel_setmem_off() failed");
            }

            empty_ = false;
        }

        arg_idx_ += 1;
    }

    /* ----------- */

    void copy_1d(rts::buffer&, size_t, rts::buffer&, size_t, size_t) override {
        format::print(std::cerr, "Error: IRIS RTS does not support D2D copy");
        std::abort();
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, void* dst, size_t len_byte) override {
        DEBUG_FMT("copy_1d: iris_task_d2h(): this={}", format::ptr(this));

#ifndef CHARM_SYCL_USE_IRIS_DMEM
        if (IRIS::iris_task_d2h(*task_, static_cast<buffer_impl<IRIS>&>(src).get(),
                                src_off_byte, len_byte, dst) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_task_d2h() failed");
        }
#else
        auto& src_ = static_cast<buffer_impl<IRIS>&>(src);

        if (src_off_byte != 0) {
            fprintf(stderr, "copy_1d: IRIS RTS does not support copy with offsets.");
            std::abort();
        }
        if (len_byte != src_.size_byte()) {
            fprintf(stderr, "copy_1d: IRIS RTS does not support partial copy.");
            std::abort();
        }

        if (IRIS::iris_data_mem_update(src_.get(), dst)) {
            throw std::runtime_error("iris_data_mem_update() failed");
        }
        if (IRIS::iris_task_dmem_flush_out(*task_, src_.get())) {
            throw std::runtime_error("iris_task_dmem_flush_out_ptr() failed");
        }
#endif
        empty_ = false;
    }

    void copy_1d(void const* src, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        DEBUG_FMT("copy_1d: iris_task_h2d(): this={}", format::ptr(this));

        if (IRIS::iris_task_h2d(*task_, static_cast<buffer_impl<IRIS>&>(dst).get(),
                                dst_off_byte, len_byte,
                                const_cast<void*>(src)) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_task_h2d() failed");
        }
        empty_ = false;
    }

    void copy_2d(rts::buffer&, size_t, size_t, rts::buffer&, size_t, size_t, size_t,
                 size_t) override {
        format::print(std::cerr, "Error: IRIS RTS does not support 2D copy");
        std::abort();
    }

    void copy_2d(rts::buffer&, size_t, size_t, void*, size_t, size_t, size_t) override {
        format::print(std::cerr, "Error: IRIS RTS does not support 2D copy");
        std::abort();
    }

    void copy_2d(void const*, size_t, rts::buffer&, size_t, size_t, size_t, size_t) override {
        format::print(std::cerr, "Error: IRIS RTS does not support 2D copy");
        std::abort();
    }

    void copy_3d(rts::buffer&, size_t, size_t, size_t, rts::buffer&, size_t, size_t, size_t,
                 size_t, size_t, size_t) override {
        format::print(std::cerr, "Error: IRIS RTS does not support 3D copy");
        std::abort();
    }

    void copy_3d(rts::buffer&, size_t, size_t, size_t, void*, size_t, size_t, size_t, size_t,
                 size_t) override {
        format::print(std::cerr, "Error: IRIS RTS does not support 3D copy");
        std::abort();
    }

    void copy_3d(void const*, size_t, size_t, rts::buffer&, size_t, size_t, size_t, size_t,
                 size_t, size_t) override {
        format::print(std::cerr, "Error: IRIS RTS does not support 3D copy");
        std::abort();
    }

    void fill(rts::buffer& dst, size_t) override {
        if (IRIS::iris_task_cmd_reset_mem(*task_, static_cast<buffer_impl<IRIS>&>(dst).get(),
                                          0x00) != IRIS::SUCCESS) {
            throw std::runtime_error("iris_task_cmd_reset_mem() failed");
        }
        empty_ = false;
    }

    /* ----------- */

    std::unique_ptr<rts::event> submit() override {
        DEBUG_FMT("submit(): this={}", format::ptr(this));

        if (kernel_) {
            DEBUG_FMT("iris_task_kernel_object: this={}", format::ptr(this));

            if (IRIS::iris_task_kernel_object(*task_, *kernel_, 3, nullptr, par_.gws.data(),
                                              par_.lws.data()) != IRIS::SUCCESS) {
                throw std::runtime_error("iris_task_kernel_object() failed");
            }
        } else if (hostfn_) {
            // TODO:
            fprintf(stderr, "Not implemented: %s: %d\n", __FILE__, __LINE__);
            std::abort();
        }

        if (!empty_) {
            DEBUG_FMT("iris_task_submit: this={}", format::ptr(this));

            if (IRIS::iris_task_submit(*task_, policy_, nullptr, 0) != IRIS::SUCCESS) {
                throw std::runtime_error("iris_task_submit");
            }
        }

        auto ev = std::make_unique<event_impl<IRIS>>(*task_, empty_);
        task_.reset();

        return ev;
    }

private:
    std::optional<task_t> task_;
    std::optional<kernel_t> kernel_;
    parallel_params par_;
    int policy_;
    std::function<void()> hostfn_;
    bool is_host_ = false;
    bool profiling_enabled = false;
    int arg_idx_ = 0;
    bool empty_ = true;
};

struct bin_info {
    char const* text;
    ssize_t len;
};

struct file {
    file() = default;

    static file mktemp(char const* ext) {
        file f;
        f.init_mkstemp(ext);
        return f;
    }

    file(file const&) = delete;

    file(file&& other) : filename(std::move(other.filename)) {
        std::swap(fd, other.fd);
    }

    file& operator=(file const&) = delete;

    file& operator=(file&& other) {
        file(std::move(other)).swap(*this);
        return *this;
    }

    ~file() {
        release();
    }

    void swap(file& other) {
        std::swap(filename, other.filename);
        std::swap(fd, other.fd);
    }

    void release() {
        if (fd >= 0) {
            unlink(filename.c_str());
        }
        fd = -1;
    }

    bool opened() const {
        return fd >= 0;
    }

    std::string filename;
    int fd = -1;

private:
    void init_mkstemp(char const* ext) {
        int err = 0;
        std::string tpl;
        tpl.reserve(256);

        tpl = "/tmp/charm-sycl-iris.XXXXXX";
        tpl += ext;

        err = try_mkstemp(tpl, ext);
        if (err == 0) {
            filename = std::move(tpl);
            return;
        }

        throw_errno_impl("mkstemps", err);
    }

    int try_mkstemp(std::string& filename, char const* ext) {
        errno = 0;
        fd = ::mkstemps(filename.data(), ::strlen(ext));
        return fd >= 0 ? 0 : errno;
    }
};

template <class IRIS>
struct subsystem_impl final : rts::subsystem {
    subsystem_impl() {
        init_logging();
        init();
    }

    subsystem_impl(subsystem_impl const&) = delete;

    subsystem_impl(subsystem_impl&&) = delete;

    subsystem_impl& operator=(subsystem_impl const&) = delete;

    subsystem_impl& operator=(subsystem_impl&&) = delete;

    void shutdown() override {
        IRIS::iris_synchronize();
        IRIS::iris_finalize();
        IRIS::close();
    }

    std::vector<std::shared_ptr<rts::platform>> get_platforms() const override {
        std::vector<std::shared_ptr<rts::platform>> plts;
        plts.push_back(std::make_shared<platform_impl<IRIS>>());
        return plts;
    }

    std::shared_ptr<rts::task> new_task() override {
        return std::make_shared<task_impl<IRIS>>();
    }

    std::unique_ptr<rts::buffer> new_buffer(void* h_ptr, size_t element_size,
                                            rts::range const& size) override {
        return std::make_unique<buffer_impl<IRIS>>(h_ptr, element_size, size);
    }

    rts::memory_domain& get_host_memory_domain() override {
        return host_;
    }

private:
    static bool is_empty(char const* str) {
        return !str || ::strlen(str) == 0;
    }

    void init() {
        DEBUG_LOG("iris: enabled");

        auto ptx = init_ptx();
        auto omp = init_openmp();
        auto hip = init_hip();

        init_iris();
    }

    file save_binary(char const* kind, char const* file_ext) {
        auto out = file::mktemp(file_ext);

        auto const* kinfo = kreg::get().find("", kreg::fnv1a(""), kind, kreg::fnv1a(kind));
        auto const* bin = reinterpret_cast<bin_info const*>(kinfo ? kinfo->fn : nullptr);

        if (!bin) {
            DEBUG_FMT("iris: {} is not found", kind);
            return {};
        }

        DEBUG_FMT("iris: {} found: ptr={} len={}", kind, format::ptr(bin->text), bin->len);

        errno = 0;
        auto const nw = ::write(out.fd, bin->text, bin->len);
        if (nw < 0 || nw != bin->len) {
            throw_errno("write");
        }

        return out;
    }

    file init_openmp() {
        if (auto const* path = ::getenv("IRIS_KERNEL_BIN_OPENMP"); !is_empty(path)) {
            return {};
        }

        auto out = save_binary("_IRIS_OMP_LOADER_", ".so");
        if (out.opened()) {
            IRIS::iris_env_set("KERNEL_BIN_OPENMP", out.filename.c_str());
        }

        return out;
    }

    file init_ptx() {
        if (auto const* path = ::getenv("IRIS_KERNEL_BIN_CUDA"); !is_empty(path)) {
            return {};
        }

        auto out = save_binary("_PTX_", ".ptx");
        if (out.opened()) {
            IRIS::iris_env_set("KERNEL_BIN_CUDA", out.filename.c_str());
        }

        return out;
    }

    file init_hip() {
        if (auto const* path = ::getenv("IRIS_KERNEL_BIN_HIP"); !is_empty(path)) {
            return {};
        }

        auto out = save_binary("_HSACO_", ".hip");
        if (out.opened()) {
            IRIS::iris_env_set("KERNEL_BIN_HIP", out.filename.c_str());
        }

        return out;
    }

    void init_iris() {
        if (IRIS::iris_init(0, nullptr, 1) != IRIS::SUCCESS) {
            throw std::runtime_error("Failed to initialize IRIS runtime");
        }
    }

    rts::host_memory_domain host_;
};

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

error::result<std::unique_ptr<rts::subsystem>>
#ifndef CHARM_SYCL_USE_IRIS_DMEM
make_iris_rts()
#else
make_iris_dmem_rts()
#endif
{
    init_logging();
    logging::timer_reset();
    fiber_init();

    CHECK_ERROR(iris_interface_20000::init());
    return std::make_unique<subsystem_impl<iris_interface_20000>>();
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
