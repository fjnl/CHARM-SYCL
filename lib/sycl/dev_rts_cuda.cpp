#include <array>
#include <cassert>
#include <cstring>
#include <cuda.h>
#include <utils/logging.hpp>
#include "dev_rts.hpp"

namespace {

LOGGING_DEFINE_SCOPE(cuda)

using namespace dev_rts;

static CUcontext g_ctx;

inline void check_cuda_error(CUresult err, char const* expr) {
    if (err != CUDA_SUCCESS) {
        std::string errmsg;
        errmsg = "CUDA Error: ";

        char const* err_str;
        cuGetErrorString(err, &err_str);
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

#define _(expr) check_cuda_error((expr), #expr)

struct custream_initializer {
    ~custream_initializer() {
        if (val) {
            _(cuStreamDestroy(val));
            val = 0;
        }
    }

    CUstream get() {
        if (!val) {
            _(cuCtxSetCurrent(g_ctx));
            _(cuStreamCreate(&val, 0));
        }
        return val;
    }

private:
    CUstream val = 0;
};

static thread_local custream_initializer tls_stream;

struct device_impl final : device_base {
    std::string info_name() const override {
        return "Dev-RTS Device [CUDA]";
    }
};

struct platform_impl final : platform_base<device_impl> {
    std::string info_name() const override {
        return "Dev-RTS Platform [CUDA]";
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
};

struct buffer_impl final : buffer_base {
    explicit buffer_impl(void* h_ptr, size_t element_size, rts::range const& size)
        : buffer_base(h_ptr, element_size, size) {
        _(cuCtxSetCurrent(g_ctx));
        _(cuMemAlloc(&devptr_, byte_size()));
    }

    ~buffer_impl() override {
        _(cuCtxSetCurrent(g_ctx));
        _(cuMemFree(devptr_));
        devptr_ = 0;
    }

    void* get_pointer() override {
        return reinterpret_cast<void*>(devptr_);
    }

    CUdeviceptr get() {
        return devptr_;
    }

private:
    CUdeviceptr devptr_ = 0;
};

struct task_impl final : rts::task,
                         std::enable_shared_from_this<task_impl>,
                         private task_parameter_storage {
    task_impl(CUmodule mod) : mod_(mod), ev_(event_node_impl::create()), wk_(ev_) {}

    void enable_profiling() override {
        DEBUG_FMT("start: enable_profiling: task[{}]", fmt::ptr(this));
        ev_->enable_profiling();
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
                          rts::memory_access ma, rts::id const& offset,
                          size_t offset_byte) override {
        auto& buf_ = dynamic_cast<buffer_impl&>(buf);
        auto const htod =
            (ma != rts::memory_access::write_only) && is_device_task_ && dom.is_host();
        auto const dtoh =
            (ma != rts::memory_access::write_only) && !is_device_task_ && !dom.is_host();

        if (htod) {
            pre_ = [h_ptr, ptr = buf_.get(), length = buf_.byte_size(),
                    next = std::move(pre_)](CUstream stream) {
                _(cuMemcpyHtoDAsync(ptr, h_ptr, length, stream));
                if (next) {
                    next(stream);
                }
            };
        } else if (dtoh) {
            pre_ = [h_ptr, ptr = buf_.get(), length = buf_.byte_size(),
                    next = std::move(pre_)](CUstream stream) {
                _(cuMemcpyDtoHAsync(h_ptr, ptr, length, stream));
                if (next) {
                    next(stream);
                }
            };
        }

        auto* ptr = next_param_ptr<CUdeviceptr>();
        auto* acc = next_param_ptr<rts::accessor>();

        *ptr = buf_.get() + offset_byte;

        acc->size[0] = buf_.get_size().size[0];
        acc->size[1] = buf_.get_size().size[1];
        acc->size[2] = buf_.get_size().size[2];
        acc->offset[0] = offset.size[0];
        acc->offset[1] = offset.size[1];
        acc->offset[2] = offset.size[2];
    }

    static inline CUdeviceptr get_ptr(rts::buffer& buf, size_t off_byte) {
        return static_cast<buffer_impl&>(buf).get() + off_byte;
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        pre_ = [dst_ptr = get_ptr(dst, dst_off_byte), src_ptr = get_ptr(src, src_off_byte),
                len_byte, prev = std::move(pre_)](CUstream stream) {
            if (prev) {
                prev(stream);
            }

            _(cuMemcpyDtoDAsync(dst_ptr, src_ptr, len_byte, stream));
        };
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, void* dst, size_t len_byte) override {
        pre_ = [dst_ptr = dst, src_ptr = get_ptr(src, src_off_byte), len_byte,
                prev = std::move(pre_)](CUstream stream) {
            if (prev) {
                prev(stream);
            }

            _(cuMemcpyDtoHAsync(dst_ptr, src_ptr, len_byte, stream));
        };
    }

    void copy_1d(void const* src, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        pre_ = [dst_ptr = get_ptr(dst, dst_off_byte), src_ptr = src, len_byte,
                prev = std::move(pre_)](CUstream stream) {
            if (prev) {
                prev(stream);
            }

            _(cuMemcpyHtoDAsync(dst_ptr, src_ptr, len_byte, stream));
        };
    }

    void copy_2d_impl(CUDA_MEMCPY2D const& desc) {
        pre_ = [desc, prev = std::move(pre_)](CUstream stream) {
            if (prev) {
                prev(stream);
            }
            _(cuMemcpy2DAsync(&desc, stream));
        };
    }

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, rts::buffer& dst,
                 size_t dst_off_byte, size_t dst_stride, size_t loop,
                 size_t len_byte) override {
        CUDA_MEMCPY2D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        desc.srcDevice = get_ptr(src, src_off_byte);
        desc.srcPitch = src_stride;
        desc.dstMemoryType = CU_MEMORYTYPE_DEVICE;
        desc.dstDevice = get_ptr(dst, dst_off_byte);
        desc.dstPitch = dst_stride;
        desc.WidthInBytes = len_byte;
        desc.Height = loop;

        copy_2d_impl(desc);
    }

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, void* dst,
                 size_t dst_stride, size_t loop, size_t len_byte) override {
        CUDA_MEMCPY2D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        desc.srcDevice = get_ptr(src, src_off_byte);
        desc.srcPitch = src_stride;
        desc.dstMemoryType = CU_MEMORYTYPE_HOST;
        desc.dstHost = dst;
        desc.dstPitch = dst_stride;
        desc.WidthInBytes = len_byte;
        desc.Height = loop;

        copy_2d_impl(desc);
    }

    void copy_2d(void const* src, size_t src_stride, rts::buffer& dst, size_t dst_off_byte,
                 size_t dst_stride, size_t loop, size_t len_byte) override {
        CUDA_MEMCPY2D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = CU_MEMORYTYPE_HOST;
        desc.srcHost = src;
        desc.srcPitch = src_stride;
        desc.dstMemoryType = CU_MEMORYTYPE_DEVICE;
        desc.dstDevice = get_ptr(dst, dst_off_byte);
        desc.dstPitch = dst_stride;
        desc.WidthInBytes = len_byte;
        desc.Height = loop;

        copy_2d_impl(desc);
    }

    void copy_3d_impl(CUDA_MEMCPY3D const& desc) {
        pre_ = [desc, prev = std::move(pre_)](CUstream stream) {
            if (prev) {
                prev(stream);
            }
            _(cuMemcpy3DAsync(&desc, stream));
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

        CUDA_MEMCPY3D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        desc.srcDevice = get_ptr(src, src_off_byte);
        desc.srcPitch = j_src_stride;
        desc.srcHeight = i_src_stride / j_src_stride;
        desc.dstMemoryType = CU_MEMORYTYPE_DEVICE;
        desc.dstDevice = get_ptr(dst, dst_off_byte);
        desc.dstPitch = j_dst_stride;
        desc.dstHeight = i_dst_stride / j_dst_stride;
        desc.WidthInBytes = len_byte;
        desc.Height = j_loop;
        desc.Depth = i_loop;

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

        CUDA_MEMCPY3D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        desc.srcDevice = get_ptr(src, src_off_byte);
        desc.srcPitch = j_src_stride;
        desc.srcHeight = i_src_stride / j_src_stride;
        desc.dstMemoryType = CU_MEMORYTYPE_HOST;
        desc.dstHost = dst;
        desc.dstPitch = j_dst_stride;
        desc.dstHeight = i_dst_stride / j_dst_stride;
        desc.WidthInBytes = len_byte;
        desc.Height = j_loop;
        desc.Depth = i_loop;

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

        CUDA_MEMCPY3D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = CU_MEMORYTYPE_HOST;
        desc.srcHost = src;
        desc.srcPitch = j_src_stride;
        desc.srcHeight = i_src_stride / j_src_stride;
        desc.dstMemoryType = CU_MEMORYTYPE_DEVICE;
        desc.dstDevice = get_ptr(dst, dst_off_byte);
        desc.dstPitch = j_dst_stride;
        desc.dstHeight = i_dst_stride / j_dst_stride;
        desc.WidthInBytes = len_byte;
        desc.Height = j_loop;
        desc.Depth = i_loop;

        copy_3d_impl(desc);
    }

    void set_kernel(char const* name, uint32_t) override {
        _(cuModuleGetFunction(&fn_, mod_, name));
        DEBUG_FMT("`{}` => {}", name, fmt::ptr(fn_));
    }

    void set_single() override {
        *next_param_ptr<size_t>() = 1;
        *next_param_ptr<size_t>() = 1;
        *next_param_ptr<size_t>() = 1;
    }

    void set_range(rts::range const& r) override {
        *next_param_ptr<size_t>() = r.size[0];
        *next_param_ptr<size_t>() = r.size[1];
        *next_param_ptr<size_t>() = r.size[2];
    }

    void set_range(rts::range const& r, rts::id const& offset) override {
        *next_param_ptr<size_t>() = r.size[0];
        *next_param_ptr<size_t>() = r.size[1];
        *next_param_ptr<size_t>() = r.size[2];
        *next_param_ptr<size_t>() = offset.size[0];
        *next_param_ptr<size_t>() = offset.size[1];
        *next_param_ptr<size_t>() = offset.size[2];
    }

    void set_nd_range(rts::nd_range const& ndr) override {
        *next_param_ptr<size_t>() = ndr.global[0] / ndr.local[0];
        *next_param_ptr<size_t>() = ndr.global[1] / ndr.local[1];
        *next_param_ptr<size_t>() = ndr.global[2] / ndr.local[2];
        *next_param_ptr<size_t>() = ndr.local[0];
        *next_param_ptr<size_t>() = ndr.local[1];
        *next_param_ptr<size_t>() = ndr.local[2];
        is_ndr_ = true;
    }

    void set_local_mem_size(size_t byte) override {
        *next_param_ptr<unsigned int*>() = nullptr;
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
        if (!is_device_task_) {
            prep_host();
        } else {
            prep_device();
        }
        return submit_();
    }

private:
    std::unique_ptr<rts::event> submit_() {
        if (pre_ || body_) {
            ev_->set_fn([task = shared_from_this()](event_ptr const& ev) mutable {
                if (task->pre_ || task->body_) {
                    auto stream = tls_stream.get();

                    if (task->pre_) {
                        task->pre_(stream);
                    }
                    if (task->body_) {
                        task->body_(stream);
                    }

                    _(cuStreamSynchronize(stream));
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
                body_ = [this](CUstream stream) {
                    auto const gz = *reinterpret_cast<size_t const*>(args_.at(0));
                    auto const gy = *reinterpret_cast<size_t const*>(args_.at(1));
                    auto const gx = *reinterpret_cast<size_t const*>(args_.at(2));
                    auto const bz = *reinterpret_cast<size_t const*>(args_.at(3));
                    auto const by = *reinterpret_cast<size_t const*>(args_.at(4));
                    auto const bx = *reinterpret_cast<size_t const*>(args_.at(5));

                    DEBUG_FMT("LaunchKernel({}, {}, {}, {}, {}, {}, {}, {})", fmt::ptr(fn_), gx,
                              gy, gz, bx, by, bz, lmem_);
                    _(cuLaunchKernel(fn_, gx, gy, gz, bx, by, bz, lmem_, stream, args_.data(),
                                     nullptr));
                };
            } else {
                body_ = [this](CUstream stream) {
                    auto const nz = *reinterpret_cast<size_t const*>(args_.at(0));
                    auto const ny = *reinterpret_cast<size_t const*>(args_.at(1));
                    auto const nx = *reinterpret_cast<size_t const*>(args_.at(2));

                    auto const gx = (nx + 255) / 256;
                    auto const gy = ny;
                    auto const gz = nz;
                    auto const bx = 256;
                    auto const by = 1;
                    auto const bz = 1;

                    DEBUG_FMT("LaunchKernel({}, {}, {}, {}, {}, {}, {}, {})", fmt::ptr(fn_), gx,
                              gy, gz, bx, by, bz, lmem_);
                    _(cuLaunchKernel(fn_, gx, gy, gz, bx, by, bz, lmem_, stream, args_.data(),
                                     nullptr));
                };
            }
        }
    }

    void prep_host() {
        if (host_fn_) {
            body_ = [this](CUstream) {
                this->host_fn_();
            };
        }
    }

    CUmodule mod_;
    bool is_device_task_ = true;
    CUfunction fn_ = nullptr;
    std::function<void()> host_fn_;
    std::function<void(CUstream)> pre_;
    std::function<void(CUstream)> body_;
    event_ptr ev_;
    weak_event_ptr wk_;
    bool is_ndr_ = false;
    size_t lmem_ = 0;
};

struct bin_info {
    char const* ptr;
    ssize_t len;
};

struct subsystem_impl final : subsystem_base<platform_impl, buffer_impl> {
    subsystem_impl() {
        init_logging();

        _(cuInit(0));
        DEBUG_LOG("enabled");

        _(cuDeviceGet(&dev_, 0));
        _(cuDevicePrimaryCtxSetFlags(dev_, CU_CTX_SCHED_SPIN));
        _(cuDevicePrimaryCtxRetain(&ctx_, dev_));
        _(cuCtxPushCurrent(ctx_));

        g_ctx = ctx_;

        auto const* fatbin = reinterpret_cast<bin_info const*>(
            kreg::get().find("", kreg::fnv1a(""), "_FATBIN_", kreg::fnv1a("_FATBIN_")));
        if (fatbin) {
            DEBUG_FMT("fatbin: ptr={} len={}", fmt::ptr(fatbin->ptr), fatbin->len);
            _(cuModuleLoadFatBinary(&mod_, fatbin->ptr));
        } else {
            DEBUG_LOG("fatbin: not found");
        }

        _(cuCtxPopCurrent(nullptr));

        q_task.reset(new BS::thread_pool_light(4));
    }

    void shutdown() override {
        q_task->wait_for_tasks();
        q_task.reset();

        CUresult err1 = CUDA_SUCCESS;
        if (mod_) {
            err1 = cuModuleUnload(mod_);
        }

        auto const err2 = cuDevicePrimaryCtxRelease(dev_);

        mod_ = nullptr;
        dev_ = CU_DEVICE_INVALID;
        ctx_ = nullptr;

        check_cuda_error(err1, "cuModuleUnload");
        check_cuda_error(err2, "cuDevicePrimaryCtxRelease");
    }

    std::shared_ptr<rts::task> new_task() override {
        return std::make_shared<task_impl>(mod_);
    }

private:
    CUmodule release() {
        auto mod = mod_;
        mod_ = nullptr;
        return mod;
    }

    CUmodule mod_ = nullptr;
    CUdevice dev_ = CU_DEVICE_INVALID;
    CUcontext ctx_ = nullptr;
};

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {
std::unique_ptr<rts::subsystem> make_dev_rts_cuda() {
    init_logging();
    utils::logging::timer_reset();
    dev_rts::init_time_point();
    return std::make_unique<subsystem_impl>();
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
