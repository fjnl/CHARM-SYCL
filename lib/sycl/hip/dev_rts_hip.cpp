#include <array>
#include <cassert>
#include <cstring>
#include <blas/rocblas_interface.hpp>
#include <blas/rocsolver_interface.hpp>
#include "../dev_rts.hpp"
#include "../interfaces.hpp"
#include "../logging.hpp"
#include "context.hpp"
#include "hip_interface.hpp"

using HIP = sycl::runtime::hip_interface_40200000;
using BLAS = sycl::runtime::rocblas_interface_40200000;
using SOL = sycl::runtime::rocsolver_interface_40200000;

namespace {

LOGGING_DEFINE_SCOPE(hip)

using namespace dev_rts;

template <class HIP>
inline void check_hip_error(typename HIP::error_t err, char const* expr) {
    if (err != HIP::k_Success) {
        std::string errmsg;
        errmsg = "HIP Error: ";

        char const* err_str = HIP::hip_get_error_string(err);
        if (err_str) {
            errmsg += err_str;
        }

        if (expr) {
            errmsg += " (";
            errmsg += expr;
            errmsg += ')';
        }

        fprintf(stderr, "%s\n", errmsg.c_str());
        std::abort();
    }
}

#define _(expr) check_hip_error<HIP>((expr), #expr)

struct device_impl final : device_base {
    std::string info_name() const override {
        return "Dev-RTS Device [HIP]";
    }
};

struct platform_impl final : platform_base<device_impl> {
    std::string info_name() const override {
        return "Dev-RTS Platform [HIP]";
    }
};

struct event_node_impl final : event_node<event_node_impl> {
    static auto create() {
        return std::make_shared<event_node_impl>();
    }
};

using event_ptr = event_node_impl::event_ptr;

using weak_event_ptr = event_node_impl::weak_event_ptr;

struct event_impl final : event_base<event_node_impl> {
    using event_base::event_base;

    auto get() const {
        return ev_;
    }
};

template <class HIP>
struct buffer_impl final : buffer_base {
    explicit buffer_impl(void* h_ptr, size_t element_size, rts::range const& size)
        : buffer_base(h_ptr, element_size, size) {
        _(HIP::hip_malloc(&devptr_, byte_size()));
    }

    ~buffer_impl() override {
        _(HIP::hip_free(std::exchange(devptr_, {})));
    }

    void* get_pointer() override {
        return reinterpret_cast<void*>(*devptr_);
    }

    typename HIP::deviceptr_t get() {
        return devptr_;
    }

private:
    typename HIP::deviceptr_t devptr_;
};

template <class HIP, class BLAS, class SOL>
struct task_impl final : rts::task,
                         std::enable_shared_from_this<task_impl<HIP, BLAS, SOL>>,
                         private task_parameter_storage {
    using module_t = typename HIP::module_t;
    using stream_t = typename HIP::stream_t;
    using deviceptr_t = typename HIP::deviceptr_t;
    using memcpy3d_t = typename HIP::memcpy3d_t;
    using function_t = typename HIP::function_t;
    using device_prop_t = typename HIP::device_prop_t;

    task_impl(module_t mod, device_prop_t const& prop)
        : mod_(mod), ev_(event_node_impl::create()), wk_(ev_), prop_(prop) {}

    void enable_profiling() override {
        DEBUG_FMT("start: enable_profiling: task[{}]", format::ptr(this));
        ev_->enable_profiling();
    }

    void depends_on(rts::event const& ev) override {
        static_cast<event_impl const&>(ev).get()->happens_before(ev_);
    }

    void depends_on(std::shared_ptr<rts::task> const& dep) override {
        assert(dep != nullptr);

        auto dep_ = dynamic_cast<task_impl const&>(*dep);

        if (auto pre = dep_.wk_.lock()) {
            pre->happens_before(ev_);
        }
    }

    void set_param(void const* ptr, size_t size) override {
        std::memcpy(next_param_ptr(size), ptr, size);
    }

    void set_buffer_param(rts::buffer& buf, void* h_ptr, rts::memory_domain const& dom,
                          rts::memory_access ma, rts::id const&, size_t offset_byte) override {
        auto& buf_ = static_cast<buffer_impl<HIP>&>(buf);
        auto const htod =
            (ma != rts::memory_access::write_only) && is_device_task_ && dom.is_host();
        auto const dtoh =
            (ma != rts::memory_access::write_only) && !is_device_task_ && !dom.is_host();

        if (htod) {
            pre_ = [h_ptr, ptr = buf_.get(), length = buf_.byte_size(),
                    next = std::move(pre_)](stream_t stream) {
                _(HIP::hip_memcpy_htod_async(ptr, h_ptr, length, stream));
                if (next) {
                    next(stream);
                }
            };
        } else if (dtoh) {
            pre_ = [h_ptr, ptr = buf_.get(), length = buf_.byte_size(),
                    next = std::move(pre_)](stream_t stream) {
                _(HIP::hip_memcpy_dtoh_async(h_ptr, ptr, length, stream));
                if (next) {
                    next(stream);
                }
            };
        }

        auto* ptr = next_param_ptr<typename deviceptr_t::native>();

        *ptr = dev_rts::advance_ptr(*buf_.get(), offset_byte);
    }

    static inline deviceptr_t get_ptr(rts::buffer& buf, size_t off_byte) {
        return deviceptr_t(
            dev_rts::advance_ptr(*static_cast<buffer_impl<HIP>&>(buf).get(), off_byte));
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        pre_ = [dst_ptr = get_ptr(dst, dst_off_byte), src_ptr = get_ptr(src, src_off_byte),
                len_byte, prev = std::move(pre_)](stream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(HIP::hip_memcpy_dtod_async(dst_ptr, src_ptr, len_byte, stream));
        };
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, void* dst, size_t len_byte) override {
        pre_ = [dst_ptr = dst, src_ptr = get_ptr(src, src_off_byte), len_byte,
                prev = std::move(pre_)](stream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(HIP::hip_memcpy_dtoh_async(dst_ptr, src_ptr, len_byte, stream));
        };
    }

    void copy_1d(void const* src, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        pre_ = [dst_ptr = get_ptr(dst, dst_off_byte), src_ptr = src, len_byte,
                prev = std::move(pre_)](stream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(HIP::hip_memcpy_htod_async(dst_ptr, const_cast<void*>(src_ptr), len_byte,
                                         stream));
        };
    }

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, rts::buffer& dst,
                 size_t dst_off_byte, size_t dst_stride, size_t loop,
                 size_t len_byte) override {
        pre_ = [src_ptr = get_ptr(src, src_off_byte), src_stride,
                dst_ptr = get_ptr(dst, dst_off_byte), dst_stride, len_byte, loop,
                prev = std::move(pre_)](stream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(HIP::hip_memcpy2d(dst_ptr, dst_stride, src_ptr, src_stride, len_byte, loop,
                                HIP::k_MemcpyDeviceToDevice));
        };
    }

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, void* dst,
                 size_t dst_stride, size_t loop, size_t len_byte) override {
        pre_ = [src_ptr = get_ptr(src, src_off_byte), src_stride, dst_ptr = dst, dst_stride,
                len_byte, loop, prev = std::move(pre_)](stream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(HIP::hip_memcpy2d(dst_ptr, dst_stride, src_ptr, src_stride, len_byte, loop,
                                HIP::k_MemcpyDeviceToHost));
        };
    }

    void copy_2d(void const* src, size_t src_stride, rts::buffer& dst, size_t dst_off_byte,
                 size_t dst_stride, size_t loop, size_t len_byte) override {
        pre_ = [src_ptr = src, src_stride, dst_ptr = get_ptr(dst, dst_off_byte), dst_stride,
                len_byte, loop, prev = std::move(pre_)](stream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(HIP::hip_memcpy2d(dst_ptr, dst_stride, src_ptr, src_stride, len_byte, loop,
                                HIP::k_MemcpyHostToDevice));
        };
    }

    void copy_3d_impl(memcpy3d_t const& desc) {
        pre_ = [desc, prev = std::move(pre_)](stream_t stream) {
            if (prev) {
                prev(stream);
            }
            _(HIP::hip_drv_memcpy3d_async(&desc, stream));
        };
    }

    void copy_3d(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                 size_t j_src_stride, rts::buffer& dst, size_t dst_off_byte,
                 size_t i_dst_stride, size_t j_dst_stride, size_t i_loop, size_t j_loop,
                 size_t len_byte) override {
        if (i_src_stride % j_src_stride != 0) {
            std::terminate();
        }
        if (i_dst_stride % j_dst_stride != 0) {
            std::terminate();
        }

        memcpy3d_t desc;

        HIP::set_srcMemoryType(desc, HIP::k_MemoryTypeDevice);
        HIP::set_srcDevice(desc, get_ptr(src, src_off_byte));
        HIP::set_srcPitch(desc, j_src_stride);
        HIP::set_srcHeight(desc, i_src_stride / j_src_stride);
        HIP::set_dstMemoryType(desc, HIP::k_MemoryTypeDevice);
        HIP::set_dstDevice(desc, get_ptr(dst, dst_off_byte));
        HIP::set_dstPitch(desc, j_dst_stride);
        HIP::set_dstHeight(desc, i_dst_stride / j_dst_stride);
        HIP::set_WidthInBytes(desc, len_byte);
        HIP::set_Height(desc, j_loop);
        HIP::set_Depth(desc, i_loop);

        copy_3d_impl(desc);
    }

    void copy_3d(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                 size_t j_src_stride, void* dst, size_t i_dst_stride, size_t j_dst_stride,
                 size_t i_loop, size_t j_loop, size_t len_byte) override {
        if (i_src_stride % j_src_stride != 0) {
            std::terminate();
        }
        if (i_dst_stride % j_dst_stride != 0) {
            std::terminate();
        }

        memcpy3d_t desc;

        HIP::set_srcMemoryType(desc, HIP::k_MemoryTypeDevice);
        HIP::set_srcDevice(desc, get_ptr(src, src_off_byte));
        HIP::set_srcPitch(desc, j_src_stride);
        HIP::set_srcHeight(desc, i_src_stride / j_src_stride);
        HIP::set_dstMemoryType(desc, HIP::k_MemoryTypeHost);
        HIP::set_dstHost(desc, dst);
        HIP::set_dstPitch(desc, j_dst_stride);
        HIP::set_dstHeight(desc, i_dst_stride / j_dst_stride);
        HIP::set_WidthInBytes(desc, len_byte);
        HIP::set_Height(desc, j_loop);
        HIP::set_Depth(desc, i_loop);

        copy_3d_impl(desc);
    }

    void copy_3d(void const* src, size_t i_src_stride, size_t j_src_stride, rts::buffer& dst,
                 size_t dst_off_byte, size_t i_dst_stride, size_t j_dst_stride, size_t i_loop,
                 size_t j_loop, size_t len_byte) override {
        if (i_src_stride % j_src_stride != 0) {
            std::terminate();
        }
        if (i_dst_stride % j_dst_stride != 0) {
            std::terminate();
        }

        memcpy3d_t desc;

        HIP::set_srcMemoryType(desc, HIP::k_MemoryTypeHost);
        HIP::set_srcHost(desc, src);
        HIP::set_srcPitch(desc, j_src_stride);
        HIP::set_srcHeight(desc, i_src_stride / j_src_stride);
        HIP::set_dstMemoryType(desc, HIP::k_MemoryTypeDevice);
        HIP::set_dstDevice(desc, get_ptr(dst, dst_off_byte));
        HIP::set_dstPitch(desc, j_dst_stride);
        HIP::set_dstHeight(desc, i_dst_stride / j_dst_stride);
        HIP::set_WidthInBytes(desc, len_byte);
        HIP::set_Height(desc, j_loop);
        HIP::set_Depth(desc, i_loop);

        copy_3d_impl(desc);
    }

    void fill(rts::buffer& dst, size_t byte_len) override {
        pre_ = [ptr = get_ptr(dst, 0), byte_len, prev = std::move(pre_)](stream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(HIP::hip_memset_async(ptr, 0x00, byte_len, stream));
        };
    }

    void set_kernel(char const* name, uint32_t) override {
        DEBUG_FMT("set_kernel({})", name);
        _(HIP::hip_module_get_function(&fn_, mod_, name));
    }

    void set_desc(rts::func_desc const* desc) override {
        desc_ = desc;
    }

    void set_single() override {
        par_[0] = 1;
        par_[1] = 1;
        par_[2] = 1;
    }

    void set_range(rts::range const& r) override {
        par_[0] = r.size[0];
        par_[1] = r.size[1];
        par_[2] = r.size[2];
    }

    void set_nd_range(rts::nd_range const& ndr) override {
        par_[0] = ndr.global[0] / ndr.local[0];
        par_[1] = ndr.global[1] / ndr.local[1];
        par_[2] = ndr.global[2] / ndr.local[2];
        par_[3] = ndr.local[0];
        par_[4] = ndr.local[1];
        par_[5] = ndr.local[2];
        is_ndr_ = true;
    }

    void set_local_mem_size(size_t byte) override {
        // *next_param_ptr<size_t*>() = nullptr;
        lmem_ = byte;
    }

    void use_device() override {
        is_device_task_ = true;
    }

    void use_host() override {
        is_device_task_ = false;
    }

    void set_device([[maybe_unused]] rts::device& dev) override {
        assert(dynamic_cast<device_impl*>(&dev) != nullptr);
    }

    void set_host(std::function<void()> const& f) override {
        host_fn_ = f;
    }

    std::unique_ptr<rts::event> submit() override {
        if (desc_) {
            prep_desc();
        } else if (!is_device_task_) {
            prep_host();
        } else {
            prep_device();
        }
        return submit_();
    }

private:
    std::unique_ptr<rts::event> submit_() {
        if (pre_ || body_) {
            ev_->set_fn([task = this->shared_from_this()](event_ptr const& ev) mutable {
                if (task->pre_ || task->body_) {
                    auto stream = sycl::runtime::hip_contexts<HIP, BLAS, SOL>::get()->strm;

                    if (task->pre_) {
                        task->pre_(stream);
                    }
                    if (task->body_) {
                        task->body_(stream);
                    }

                    _(HIP::hip_stream_synchronize(stream));
                }

                ev->complete();
            });
        }
        ev_->finalize();

        return std::make_unique<event_impl>(std::move(ev_));
    }

    void prep_device() {
        if (fn_) {
            if (is_ndr_) {
                body_ = [this](stream_t stream) {
                    auto const gz = par_[0];
                    auto const gy = par_[1];
                    auto const gx = par_[2];
                    auto const bz = par_[3];
                    auto const by = par_[4];
                    auto const bx = par_[5];

                    DEBUG_FMT("hipModuleLaunchKernel({}, {}, {}, {}, {}, {}, {}, {})",
                              format::ptr(*fn_), gx, gy, gz, bx, by, bz, lmem_);

                    _(HIP::hip_module_launch_kernel(fn_, gx, gy, gz, bx, by, bz, lmem_, stream,
                                                    args_.data(), nullptr));
                };
            } else {
                body_ = [this](stream_t stream) {
                    unsigned gx, gy, gz, bx, by, bz;
                    heuristic(gx, gy, gz, bx, by, bz);

                    DEBUG_FMT("hipModuleLaunchKernel({}, {}, {}, {}, {}, {}, {}, {})",
                              format::ptr(*fn_), gx, gy, gz, bx, by, bz, lmem_);

                    _(HIP::hip_module_launch_kernel(fn_, gx, gy, gz, bx, by, bz, lmem_, stream,
                                                    args_.data(), nullptr));
                };
            }
        }
    }

    void heuristic(unsigned& gx, unsigned& gy, unsigned& gz, unsigned& bx, unsigned& by,
                   unsigned& bz) const {
        auto const n_cu = HIP::get_multiProcessorCount(prop_);

        gx = 1;
        bx = 1;
        gy = par_[1];
        by = 1;
        gz = par_[0];
        bz = 1;

        if (par_[0] == 1 && par_[1] == 1) {
            auto const n = par_[2];
            bx = 512u;
            gx = std::min<unsigned int>(n_cu * 3, (n + bx - 1) / bx);
        } else {
            auto const n = par_[2];
            bx = std::min<unsigned int>(n, 512);
            gx = (n + bx - 1) / bx;
        }
    }

    void prep_host() {
        if (host_fn_) {
            body_ = [this](stream_t) {
                this->host_fn_();
            };
        }
    }

    void prep_desc() {
        body_ = [this](stream_t strm) {
            auto const is_rocblas = desc_->hip_tag == &sycl::blas::rocblas_tag;
            auto const is_rocsolver = desc_->hip_tag == &sycl::blas::rocsolver_tag;

            if (is_rocblas || is_rocsolver) {
                auto* workspace = sycl::runtime::hip_contexts<HIP, BLAS, SOL>::get();

                if (is_rocblas) {
                    *next_param_ptr<typename BLAS::handle_t>() = workspace->blas_handle.get();
                } else {
                    *next_param_ptr<typename BLAS::handle_t>() = workspace->blas_handle.get();
                    add_param_val(workspace->d_info.get());
                }
            } else {
                *next_param_ptr<stream_t>() = strm;
            }

            desc_->hip(args_.data());
        };
    }

    module_t mod_;
    bool is_device_task_ = true;
    function_t fn_;
    std::function<void()> host_fn_;
    std::function<void(stream_t)> pre_;
    std::function<void(stream_t)> body_;
    std::array<size_t, 6> par_;
    event_ptr ev_;
    weak_event_ptr wk_;
    bool is_ndr_ = false;
    size_t lmem_ = 0;
    rts::func_desc const* desc_ = nullptr;
    device_prop_t const& prop_;
};

struct bin_info {
    char const* ptr;
    ssize_t len;
};

template <class HIP, class BLAS, class SOL>
struct subsystem_impl final : subsystem_base<platform_impl, buffer_impl<HIP>> {
    subsystem_impl() {
        init_logging();

        HIP::init();

        _(HIP::hip_init(0));
        DEBUG_LOG("enabled");

        auto const* kinfo =
            kreg::get().find("", kreg::fnv1a(""), "_HSACO_", kreg::fnv1a("_HSACO_"));
        auto const* hsaco = reinterpret_cast<bin_info const*>(kinfo ? kinfo->fn : nullptr);
        if (hsaco) {
            DEBUG_FMT("hsaco: ptr={} len={}", format::ptr(hsaco->ptr), hsaco->len);
            _(HIP::hip_module_load_data(&mod_, hsaco->ptr));
        } else {
            DEBUG_LOG("hsaco: not found");
        }

        _(HIP::hip_get_device_properties(&prop_, 0));

        int constexpr n_threads = 4;
        sycl::runtime::hip_contexts<HIP, BLAS, SOL>::workspaces.resize(n_threads);
        q_task.reset(new BS::thread_pool(n_threads));
    }

    void shutdown() override {
        q_task->wait();
        q_task.reset();
        sycl::runtime::hip_contexts<HIP, BLAS, SOL>::workspaces.clear();

        if (auto const mod = std::exchange(mod_, {})) {
            _(HIP::hip_module_unload(mod));
        }

        HIP::close();
    }

    std::shared_ptr<rts::task> new_task() override {
        return std::make_shared<task_impl<HIP, BLAS, SOL>>(mod_, prop_);
    }

private:
    typename HIP::module_t mod_;
    typename HIP::device_prop_t prop_;
};

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {
error::result<std::unique_ptr<rts::subsystem>> make_dev_rts_hip() {
    init_logging();
    logging::timer_reset();
    dev_rts::init_time_point();

    CHECK_ERROR(HIP::init());
    return std::make_unique<subsystem_impl<HIP, BLAS, SOL>>();
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
