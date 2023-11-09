#include <array>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <assert.h>
#include <fmt/format.h>
#include <iris/iris.h>
#include <unistd.h>
#include <utils/logging.hpp>
#include "../fiber.hpp"
#include "../kreg.hpp"
#include "../rts.hpp"

namespace {

LOGGING_DEFINE_SCOPE(iris)

bool USE_IRIS_ALL = false;
bool USE_IRIS_ANY = false;
bool USE_IRIS_ROUNDROBIN = false;
bool USE_IRIS_CPU = false;
bool USE_IRIS_GPU = false;

namespace rts = CHARM_SYCL_NS::rts;

[[noreturn]] void throw_errno_impl(char const* errmsg, int errno_,
                                   char const* file = __builtin_FILE(),
                                   int line = __builtin_LINE()) {
    auto const what = fmt::format("Errot at {}:{}: {}", file, line, errmsg);
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

struct device_impl : rts::device {
    explicit device_impl(int policy) : policy_(policy) {
        init();
    }

    device_impl(device_impl const&) = delete;

    device_impl(device_impl&&) = delete;

    device_impl& operator=(device_impl const&) = delete;

    device_impl& operator=(device_impl&&) = delete;

    bool is_cpu() const override {
        return types_ & iris_cpu;
    }

    bool is_gpu() const override {
        return types_ & iris_gpu;
    }

    bool is_accelerator() const override {
        return types_ & (iris_gpu | iris_fpga);
    }

    bool is_fpga() const override {
        return types_ & iris_fpga;
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
        iris_device_info(0, iris_name, &info, &size);
        switch (policy_) {
            case iris_cpu:
                type = "IRIS CPU Device";
                break;
            case iris_gpu:
                type = "IRIS GPU Device";
                break;
            case iris_fpga:
                type = "IRIS FPGA Device";
                break;
            case iris_any:
                type = "IRIS Any Device";
                break;
            case iris_all:
                type = "IRIS All Device";
                break;
            case iris_roundrobin:
                type = "IRIS Roundrobin Device";
                break;
            default:
                type = "IRIS Unknown Device";
                break;
        }
        return std::string(info) + " (" + type + ")";
    }

    std::string info_vendor() const override {
        char vendor[256];
        size_t size;
        iris_device_info(0, iris_vendor, vendor, &size);
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
        return type & (iris_any | iris_all | iris_roundrobin);
    }

    void init() {
        int n = 0;
        if (iris_device_count(&n) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_device_count() failed");
        }

        for (int i = 0; i < n; i++) {
            int type = -1;

            if (iris_device_info(i, iris_type, &type, nullptr) != IRIS_SUCCESS) {
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

struct platform_impl : rts::platform {
    platform_impl() {
        check_env();

        std::vector<unsigned int> policies;

        if (USE_IRIS_ALL) {
            policies.push_back(iris_all);
        }
        if (USE_IRIS_ANY) {
            policies.push_back(iris_any);
        }
        if (USE_IRIS_ROUNDROBIN) {
            policies.push_back(iris_roundrobin);
        }
        if (USE_IRIS_GPU) {
            policies.push_back(iris_gpu);
        }
        if (USE_IRIS_CPU) {
            policies.push_back(iris_cpu);
        }

        if (policies.empty()) {
            policies.push_back(iris_gpu);
            policies.push_back(iris_cpu);
            // policies.push_back(iris_fpga);
        }

        for (auto const& policy : policies) {
            if (auto d = std::make_shared<device_impl>(policy); d->available()) {
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
        iris_platform_info(0, iris_name, name, &size);
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
            USE_IRIS_ALL = false;
            USE_IRIS_ANY = false;
            USE_IRIS_ROUNDROBIN = false;
            USE_IRIS_CPU = false;
            USE_IRIS_GPU = false;

            if (strcasecmp(policy, "any") == 0) {
                USE_IRIS_ANY = true;
            } else if (strcasecmp(policy, "all") == 0) {
                USE_IRIS_ALL = true;
            } else if (strcasecmp(policy, "roundrobin") == 0) {
                USE_IRIS_ROUNDROBIN = true;
            } else if (strcasecmp(policy, "cpu") == 0) {
                USE_IRIS_CPU = true;
            } else if (strcasecmp(policy, "gpu") == 0) {
                USE_IRIS_GPU = true;
            } else {
                throw std::runtime_error(
                    fmt::format("Unknown CHARM_SYCL_IRIS_POLICY value: {}", policy));
            }
        }
    }

    std::vector<std::shared_ptr<rts::device>> devs_;
};

struct buffer_impl final : rts::buffer {
    explicit buffer_impl(void*, size_t element_size, rts::range const& size)
        : element_size_(element_size), size_(size) {
        if (iris_mem_create(size_byte(), &mem_) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_mem_create() failed");
        }
    }

    buffer_impl(buffer_impl const&) = delete;

    buffer_impl(buffer_impl&&) = delete;

    buffer_impl& operator=(buffer_impl const&) = delete;

    buffer_impl& operator=(buffer_impl&&) = delete;

    ~buffer_impl() {
        // iris_mem_release(mem_);
    }

    void* get_pointer() override {
        return &mem_;
    }

    void* get_host_pointer() override {
        return h_ptr_;
    }

    iris_mem& get() {
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
    iris_mem mem_;
    size_t element_size_;
    rts::range size_;
    void* h_ptr_ = nullptr;
};

struct event_barrier_impl final : rts::event_barrier {
    void wait() override {
        if (need_sync_) {
            if (iris_synchronize() != IRIS_SUCCESS) {
                throw std::runtime_error("iris_synchronize() failed");
            }
        }
    }

    void add(sycl::runtime::event& ev) override;

private:
    bool need_sync_ = false;
};

struct event_impl final : rts::event {
    explicit event_impl([[maybe_unused]] iris_task const& task, bool empty)
        :
#ifdef HAVE_IRIS_TASK_SUBMIT
          task_(task),
#endif
          empty_(empty) {
    }

    ~event_impl() {}

    event_impl(event_impl const&) = delete;

    event_impl(event_impl&&) = delete;

    event_impl& operator=(event_impl const&) = delete;

    event_impl& operator=(event_impl&&) = delete;

    std::unique_ptr<sycl::runtime::event_barrier> create_barrier() override {
        return std::make_unique<event_barrier_impl>();
    }

    bool is_empty() const {
        return empty_;
    }

    uint64_t profiling_command_submit() override {
        iris_synchronize();

#ifdef HAVE_IRIS_TASK_SUBMIT
        uint64_t submission_start_time;
        iris_task_info(task_, iris_task_time_submit, &submission_start_time, nullptr);
        return submission_start_time;
#else
        return -1;
#endif
    }

    uint64_t profiling_command_start() override {
        iris_synchronize();

#ifdef HAVE_IRIS_TASK_SUBMIT
        size_t kernel_start_time;
        iris_task_info(task_, iris_task_time_start, &kernel_start_time, nullptr);
        return kernel_start_time;
#else
        return -1;
#endif
    }

    uint64_t profiling_command_end() override {
        iris_synchronize();

#ifdef HAVE_IRIS_TASK_SUBMIT
        size_t kernel_end_time;
        iris_task_info(task_, iris_task_time_end, &kernel_end_time, nullptr);
        return kernel_end_time;
#else
        return -1;
#endif
    }

private:
#ifdef HAVE_IRIS_TASK_SUBMIT
    iris_task task_;
#endif
    bool empty_;
};

void event_barrier_impl::add(sycl::runtime::event& ev) {
    if (!static_cast<event_impl&>(ev).is_empty()) {
        need_sync_ = true;
    }
}

struct task_impl final : rts::task, std::enable_shared_from_this<task_impl> {
    explicit task_impl() {
        task_.emplace();
        if (iris_task_create(&*task_) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_task_create() failed");
        }
    }

    ~task_impl() {
        if (profiling_enabled) {
            iris_task_release(*task_);
        }
    }

    task_impl(task_impl const&) = delete;

    task_impl(task_impl&&) = delete;

    task_impl& operator=(task_impl const&) = delete;

    task_impl& operator=(task_impl&&) = delete;

    /* ----------- */

    void enable_profiling() override {
        profiling_enabled = true;
        iris_task_retain(*task_, true);
    }

    void depends_on(std::shared_ptr<rts::task> const& dep) override {
        if (iris_task_depend(*task_, 1, &*std::dynamic_pointer_cast<task_impl>(dep)->task_) !=
            IRIS_SUCCESS) {
            throw std::runtime_error("iris_task_depend() failed");
        }
    }

    /* ----------- */

    void use_device() override {
        is_host_ = false;
    }

    void set_device(rts::device& dev) override {
        policy_ = dynamic_cast<device_impl&>(dev).policy();
    }

    /* ----------- */

    void use_host() override {
        is_host_ = true;
        policy_ = iris_cpu;
    }

    /* ----------- */

    void set_kernel(char const* name, uint32_t) override {
        kernel_.emplace();
        if (iris_kernel_create(name, &*kernel_) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_kernel_create() failed");
        }
        empty_ = false;
    }

    /* ----------- */

    void set_host(std::function<void()> const& f) override {
        assert(is_host_ == true);
        hostfn_ = f;
        if (f) {
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

        set_range_params(1, 1, 1);
    }

    void set_range(rts::range const& r) override {
        par_.gws[0] = ((r.size[2] + 255) / 256) * 256;
        par_.gws[1] = r.size[1];
        par_.gws[2] = r.size[0];

        par_.lws[0] = 256;
        par_.lws[1] = 1;
        par_.lws[2] = 1;

        set_range_params(r.size[0], r.size[1], r.size[2]);
    }

    void set_range(rts::range const& r, rts::id const& offset) override {
        par_.gws[0] = ((r.size[2] + 255) / 256) * 256;
        par_.gws[1] = r.size[1];
        par_.gws[2] = r.size[0];

        par_.lws[0] = 256;
        par_.lws[1] = 1;
        par_.lws[2] = 1;

        set_range_params(r.size[0], r.size[1], r.size[2]);
        set_range_params(offset.size[0], offset.size[1], offset.size[2]);
    }

    void set_range_params(size_t size0, size_t size1, size_t size2) {
        if (iris_kernel_setarg(*kernel_, arg_idx_, sizeof(size0),
                               const_cast<size_t*>(&size0)) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_kernel_setarg() failed");
        }
        if (iris_kernel_setarg(*kernel_, arg_idx_ + 1, sizeof(size1),
                               const_cast<size_t*>(&size1)) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_kernel_setarg() failed");
        }
        if (iris_kernel_setarg(*kernel_, arg_idx_ + 2, sizeof(size2),
                               const_cast<size_t*>(&size2)) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_kernel_setarg() failed");
        }

        arg_idx_ += 3;
    }

    void set_nd_range(rts::nd_range const& ndr) override {
        par_.gws[0] = ndr.global[2];
        par_.gws[1] = ndr.global[1];
        par_.gws[2] = ndr.global[0];

        set_range_params(ndr.global[0] / ndr.local[0], ndr.global[1] / ndr.local[1],
                         ndr.global[2] / ndr.local[2]);

        par_.lws[0] = ndr.local[2];
        par_.lws[1] = ndr.local[1];
        par_.lws[2] = ndr.local[0];

        set_range_params(ndr.local[0], ndr.local[1], ndr.local[2]);
    }

    void set_local_mem_size(size_t byte) override {
        if (byte > 0) {
            // TODO
            fprintf(stderr, "Error: Local memory is not supported on IRIS RTS\n");
            std::abort();
        }

        unsigned int* p = nullptr;
        set_param(&p, sizeof(p));
    }

    /* ----------- */

    void set_param(void const* ptr, size_t size) override {
        if (kernel_) {
            if (iris_kernel_setarg(*kernel_, arg_idx_, size, const_cast<void*>(ptr)) !=
                IRIS_SUCCESS) {
                throw std::runtime_error("iris_kernel_setarg() failed");
            }
        }

        arg_idx_++;
    }

    void set_buffer_param(rts::buffer& buf, void* h_ptr, rts::memory_domain const& dom,
                          rts::memory_access ma, rts::id const& offset,
                          size_t offset_byte) override {
        auto& buf_ = dynamic_cast<buffer_impl&>(buf);
        auto const htod = (ma != rts::memory_access::write_only) && !is_host_ && dom.is_host();
        auto const dtoh = (ma != rts::memory_access::write_only) && is_host_ && !dom.is_host();

        DEBUG_FMT(
            "set_buffer_param(h_ptr={}, dom={}, ma={}, off=[{}, {}, {}], off_byte={}) htod={} "
            "dtoh={}",
            fmt::ptr(h_ptr), dom.id(), static_cast<int>(ma), offset.size[0], offset.size[1],
            offset.size[2], offset_byte, htod, dtoh);

        if (htod) {
            if (iris_task_h2d(*task_, buf_.get(), buf_.offset_byte(), buf_.size_byte(),
                              h_ptr) != IRIS_SUCCESS) {
                throw std::runtime_error("iris_task_h2d() failed");
            }
            empty_ = false;
        } else if (dtoh) {
            if (iris_task_d2h(*task_, buf_.get(), buf_.offset_byte(), buf_.size_byte(),
                              h_ptr) != IRIS_SUCCESS) {
                throw std::runtime_error("iris_task_d2h() failed");
            }
            empty_ = false;
        }

        if (kernel_) {
            static_assert(iris_r != 0 && iris_w != 0 && iris_rw != 0);
            size_t mode = 0;

            switch (ma) {
                case rts::memory_access::read_only:
                    mode = iris_r;
                    break;

                case rts::memory_access::write_only:
                    mode = iris_w;
                    break;

                default:
                    mode = iris_rw;
                    break;
            }

            if (iris_kernel_setmem_off(*kernel_, arg_idx_, buf_.get(), offset_byte, mode) !=
                IRIS_SUCCESS) {
                throw std::runtime_error("iris_kernel_setmem_off() failed");
            }

            rts::accessor acc;
            acc.size[0] = buf_.get_size().size[0];
            acc.size[1] = buf_.get_size().size[1];
            acc.size[2] = buf_.get_size().size[2];
            acc.offset[0] = offset.size[0];
            acc.offset[1] = offset.size[1];
            acc.offset[2] = offset.size[2];

            if (iris_kernel_setarg(*kernel_, arg_idx_ + 1, sizeof(acc), &acc) != IRIS_SUCCESS) {
                throw std::runtime_error("iris_kernel_setarg() failed");
            }

            empty_ = false;
        }

        arg_idx_ += 2;
    }

    /* ----------- */

    void copy_1d(rts::buffer&, size_t, rts::buffer&, size_t, size_t) override {
        fmt::print(stderr, "Error: IRIS RTS does not support D2D copy");
        std::abort();
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, void* dst, size_t len_byte) override {
        DEBUG_FMT("copy_1d: iris_task_d2h(): this={}", fmt::ptr(this));

        if (iris_task_d2h(*task_, static_cast<buffer_impl&>(src).get(), src_off_byte, len_byte,
                          dst) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_task_d2h() failed");
        }
        empty_ = false;
    }

    void copy_1d(void const* src, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        DEBUG_FMT("copy_1d: iris_task_h2d(): this={}", fmt::ptr(this));

        if (iris_task_h2d(*task_, static_cast<buffer_impl&>(dst).get(), dst_off_byte, len_byte,
                          const_cast<void*>(src)) != IRIS_SUCCESS) {
            throw std::runtime_error("iris_task_h2d() failed");
        }
        empty_ = false;
    }

    void copy_2d(rts::buffer&, size_t, size_t, rts::buffer&, size_t, size_t, size_t,
                 size_t) override {
        fmt::print(stderr, "Error: IRIS RTS does not support 2D copy");
        std::abort();
    }

    void copy_2d(rts::buffer&, size_t, size_t, void*, size_t, size_t, size_t) override {
        fmt::print(stderr, "Error: IRIS RTS does not support 2D copy");
        std::abort();
    }

    void copy_2d(void const*, size_t, rts::buffer&, size_t, size_t, size_t, size_t) override {
        fmt::print(stderr, "Error: IRIS RTS does not support 2D copy");
        std::abort();
    }

    void copy_3d(rts::buffer&, size_t, size_t, size_t, rts::buffer&, size_t, size_t, size_t,
                 size_t, size_t, size_t) override {
        fmt::print(stderr, "Error: IRIS RTS does not support 3D copy");
        std::abort();
    }

    void copy_3d(rts::buffer&, size_t, size_t, size_t, void*, size_t, size_t, size_t, size_t,
                 size_t) override {
        fmt::print(stderr, "Error: IRIS RTS does not support 3D copy");
        std::abort();
    }

    void copy_3d(void const*, size_t, size_t, rts::buffer&, size_t, size_t, size_t, size_t,
                 size_t, size_t) override {
        fmt::print(stderr, "Error: IRIS RTS does not support 3D copy");
        std::abort();
    }

    /* ----------- */

    std::unique_ptr<rts::event> submit() override {
        DEBUG_FMT("submit(): this={}", fmt::ptr(this));

        if (kernel_) {
            DEBUG_FMT("iris_task_kernel_object: this={}", fmt::ptr(this));

            if (iris_task_kernel_object(*task_, *kernel_, 3, nullptr, par_.gws.data(),
                                        par_.lws.data()) != IRIS_SUCCESS) {
                throw std::runtime_error("iris_task_kernel_object() failed");
            }
        } else if (hostfn_) {
            // TODO:
            fprintf(stderr, "Not implemented: %s: %d\n", __FILE__, __LINE__);
            std::abort();
        }

        if (!empty_) {
            DEBUG_FMT("iris_task_submit: this={}", fmt::ptr(this));

            if (iris_task_submit(*task_, policy_, nullptr, 0) != IRIS_SUCCESS) {
                throw std::runtime_error("iris_task_submit");
            }
        }

        auto ev = std::make_unique<event_impl>(*task_, empty_);
        task_.reset();

        return ev;
    }

private:
    std::optional<iris_task> task_;
    std::optional<iris_kernel> kernel_;
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
        iris_synchronize();
        iris_finalize();
    }

    std::vector<std::shared_ptr<rts::platform>> get_platforms() const override {
        std::vector<std::shared_ptr<rts::platform>> plts;
        plts.push_back(std::make_shared<platform_impl>());
        return plts;
    }

    std::shared_ptr<rts::task> new_task() override {
        return std::make_shared<task_impl>();
    }

    std::unique_ptr<rts::buffer> new_buffer(void* h_ptr, size_t element_size,
                                            rts::range const& size) override {
        return std::make_unique<buffer_impl>(h_ptr, element_size, size);
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
        auto* bin = reinterpret_cast<bin_info const*>(
            kreg::get().find("", kreg::fnv1a(""), kind, kreg::fnv1a(kind)));

        if (!bin) {
            DEBUG_FMT("iris: {} is not found", kind);
            return {};
        }

        DEBUG_FMT("iris: {} found: ptr={} len={}", kind, fmt::ptr(bin->text), bin->len);

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
            iris_env_set("KERNEL_BIN_OPENMP", out.filename.c_str());
        }

        return out;
    }

    file init_ptx() {
        if (auto const* path = ::getenv("IRIS_KERNEL_BIN_CUDA"); !is_empty(path)) {
            return {};
        }

        auto out = save_binary("_PTX_", ".ptx");
        if (out.opened()) {
            iris_env_set("KERNEL_BIN_CUDA", out.filename.c_str());
        }

        return out;
    }

    file init_hip() {
        if (auto const* path = ::getenv("IRIS_KERNEL_BIN_HIP"); !is_empty(path)) {
            return {};
        }

        auto out = save_binary("_HSACO_", ".hip");
        if (out.opened()) {
            iris_env_set("KERNEL_BIN_HIP", out.filename.c_str());
        }

        return out;
    }

    void init_iris() {
        if (iris_init(0, nullptr, 1) != IRIS_SUCCESS) {
            throw std::runtime_error("Failed to initialize IRIS runtime");
        }
    }

    rts::host_memory_domain host_;
};

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

std::unique_ptr<rts::subsystem> make_iris_rts() {
    init_logging();
    utils::logging::timer_reset();
    fiber_init();
    return std::make_unique<subsystem_impl>();
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
