#include <array>
#include <cassert>
#include <cstring>
#include <hip/hip_runtime_api.h>
#include <utils/logging.hpp>
#include "dev_rts.hpp"

namespace {

LOGGING_DEFINE_SCOPE(hip)

using namespace dev_rts;

inline void check_hip_error(hipError_t err, char const* expr) {
    if (err != hipSuccess) {
        std::string errmsg;
        errmsg = "HIP Error: ";

        char const* err_str = hipGetErrorString(err);
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

#define _(expr) check_hip_error((expr), #expr)

struct stream_initializer {
    ~stream_initializer() {
        if (val_) {
            _(hipStreamDestroy(val_));
            val_ = 0;
        }
    }

    hipStream_t get() {
        if (!val_) {
            _(hipStreamCreate(&val_));
        }
        return val_;
    }

private:
    hipStream_t val_ = 0;
};

static thread_local stream_initializer tls_stream;

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
};

struct buffer_impl final : buffer_base {
    explicit buffer_impl(void* h_ptr, size_t element_size, rts::range const& size)
        : buffer_base(h_ptr, element_size, size) {
        _(hipMalloc(&devptr_, byte_size()));
    }

    ~buffer_impl() override {
        _(hipFree(std::exchange(devptr_, nullptr)));
    }

    void* get_pointer() override {
        return reinterpret_cast<void*>(devptr_);
    }

    hipDeviceptr_t get() {
        return devptr_;
    }

private:
    void* devptr_ = nullptr;
};

struct task_impl final : rts::task,
                         std::enable_shared_from_this<task_impl>,
                         private task_parameter_storage {
    task_impl(hipModule_t mod) : mod_(mod), ev_(event_node_impl::create()), wk_(ev_) {}

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
                    next = std::move(pre_)](hipStream_t stream) {
                _(hipMemcpyHtoDAsync(ptr, h_ptr, length, stream));
                if (next) {
                    next(stream);
                }
            };
        } else if (dtoh) {
            pre_ = [h_ptr, ptr = buf_.get(), length = buf_.byte_size(),
                    next = std::move(pre_)](hipStream_t stream) {
                _(hipMemcpyDtoHAsync(h_ptr, ptr, length, stream));
                if (next) {
                    next(stream);
                }
            };
        }

        auto* ptr = next_param_ptr<void*>();
        auto* acc = next_param_ptr<rts::accessor>();

        *ptr = dev_rts::advance_ptr(buf_.get(), offset_byte);

        acc->size[0] = buf_.get_size().size[0];
        acc->size[1] = buf_.get_size().size[1];
        acc->size[2] = buf_.get_size().size[2];
        acc->offset[0] = offset.size[0];
        acc->offset[1] = offset.size[1];
        acc->offset[2] = offset.size[2];
    }

    static inline hipDeviceptr_t get_ptr(rts::buffer& buf, size_t off_byte) {
        return dev_rts::advance_ptr(static_cast<buffer_impl&>(buf).get(), off_byte);
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        pre_ = [dst_ptr = get_ptr(dst, dst_off_byte), src_ptr = get_ptr(src, src_off_byte),
                len_byte, prev = std::move(pre_)](hipStream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(hipMemcpyDtoDAsync(dst_ptr, src_ptr, len_byte, stream));
        };
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, void* dst, size_t len_byte) override {
        pre_ = [dst_ptr = dst, src_ptr = get_ptr(src, src_off_byte), len_byte,
                prev = std::move(pre_)](hipStream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(hipMemcpyDtoHAsync(dst_ptr, src_ptr, len_byte, stream));
        };
    }

    void copy_1d(void const* src, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        pre_ = [dst_ptr = get_ptr(dst, dst_off_byte), src_ptr = src, len_byte,
                prev = std::move(pre_)](hipStream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(hipMemcpyHtoDAsync(dst_ptr, const_cast<void*>(src_ptr), len_byte, stream));
        };
    }

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, rts::buffer& dst,
                 size_t dst_off_byte, size_t dst_stride, size_t loop,
                 size_t len_byte) override {
        pre_ = [src_ptr = get_ptr(src, src_off_byte), src_stride,
                dst_ptr = get_ptr(dst, dst_off_byte), dst_stride, len_byte, loop,
                prev = std::move(pre_)](hipStream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(hipMemcpy2D(dst_ptr, dst_stride, src_ptr, src_stride, len_byte, loop,
                          hipMemcpyDeviceToDevice));
        };
    }

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, void* dst,
                 size_t dst_stride, size_t loop, size_t len_byte) override {
        pre_ = [src_ptr = get_ptr(src, src_off_byte), src_stride, dst_ptr = dst, dst_stride,
                len_byte, loop, prev = std::move(pre_)](hipStream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(hipMemcpy2D(dst_ptr, dst_stride, src_ptr, src_stride, len_byte, loop,
                          hipMemcpyDeviceToHost));
        };
    }

    void copy_2d(void const* src, size_t src_stride, rts::buffer& dst, size_t dst_off_byte,
                 size_t dst_stride, size_t loop, size_t len_byte) override {
        pre_ = [src_ptr = src, src_stride, dst_ptr = get_ptr(dst, dst_off_byte), dst_stride,
                len_byte, loop, prev = std::move(pre_)](hipStream_t stream) {
            if (prev) {
                prev(stream);
            }

            _(hipMemcpy2D(dst_ptr, dst_stride, src_ptr, src_stride, len_byte, loop,
                          hipMemcpyHostToDevice));
        };
    }

    void copy_3d_impl(HIP_MEMCPY3D const& desc) {
        pre_ = [desc, prev = std::move(pre_)](hipStream_t stream) {
            if (prev) {
                prev(stream);
            }
            _(hipDrvMemcpy3DAsync(&desc, stream));
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

        HIP_MEMCPY3D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = hipMemoryTypeDevice;
        desc.srcDevice = get_ptr(src, src_off_byte);
        desc.srcPitch = j_src_stride;
        desc.srcHeight = i_src_stride / j_src_stride;
        desc.dstMemoryType = hipMemoryTypeDevice;
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

        HIP_MEMCPY3D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = hipMemoryTypeDevice;
        desc.srcDevice = get_ptr(src, src_off_byte);
        desc.srcPitch = j_src_stride;
        desc.srcHeight = i_src_stride / j_src_stride;
        desc.dstMemoryType = hipMemoryTypeHost;
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

        HIP_MEMCPY3D desc;
        memset(&desc, 0x00, sizeof(desc));

        desc.srcMemoryType = hipMemoryTypeHost;
        desc.srcHost = src;
        desc.srcPitch = j_src_stride;
        desc.srcHeight = i_src_stride / j_src_stride;
        desc.dstMemoryType = hipMemoryTypeDevice;
        desc.dstDevice = get_ptr(dst, dst_off_byte);
        desc.dstPitch = j_dst_stride;
        desc.dstHeight = i_dst_stride / j_dst_stride;
        desc.WidthInBytes = len_byte;
        desc.Height = j_loop;
        desc.Depth = i_loop;

        copy_3d_impl(desc);
    }

    void set_kernel(char const* name, uint32_t) override {
        _(hipModuleGetFunction(&fn_, mod_, name));
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

                    _(hipStreamSynchronize(stream));
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
                body_ = [this](hipStream_t stream) {
                    auto const gz = *reinterpret_cast<size_t const*>(args_.at(0));
                    auto const gy = *reinterpret_cast<size_t const*>(args_.at(1));
                    auto const gx = *reinterpret_cast<size_t const*>(args_.at(2));
                    auto const bz = *reinterpret_cast<size_t const*>(args_.at(3));
                    auto const by = *reinterpret_cast<size_t const*>(args_.at(4));
                    auto const bx = *reinterpret_cast<size_t const*>(args_.at(5));

                    DEBUG_FMT("hipModuleLaunchKernel({}, {}, {}, {}, {}, {}, {}, {})",
                              fmt::ptr(fn_), gx, gy, gz, bx, by, bz, lmem_);

                    _(hipModuleLaunchKernel(fn_, gx, gy, gz, bx, by, bz, lmem_, stream,
                                            args_.data(), nullptr));
                };
            } else {
                body_ = [this](hipStream_t stream) {
                    auto nz = *reinterpret_cast<size_t const*>(args_.at(0));
                    auto ny = *reinterpret_cast<size_t const*>(args_.at(1));
                    auto nx = *reinterpret_cast<size_t const*>(args_.at(2));

                    auto const gx = (nx + 255) / 256;
                    auto const gy = ny;
                    auto const gz = nz;
                    auto const bx = 256;
                    auto const by = 1;
                    auto const bz = 1;

                    DEBUG_FMT("hipModuleLaunchKernel({}, {}, {}, {}, {}, {}, {}, {})",
                              fmt::ptr(fn_), gx, gy, gz, bx, by, bz, lmem_);

                    _(hipModuleLaunchKernel(fn_, gx, gy, gz, bx, by, bz, lmem_, stream,
                                            args_.data(), nullptr));
                };
            }
        }
    }

    void prep_host() {
        if (host_fn_) {
            body_ = [this](hipStream_t) {
                this->host_fn_();
            };
        }
    }

    hipModule_t mod_;
    bool is_device_task_ = true;
    hipFunction_t fn_ = nullptr;
    std::function<void()> host_fn_;
    std::function<void(hipStream_t)> pre_;
    std::function<void(hipStream_t)> body_;
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

        _(hipInit(0));
        DEBUG_LOG("enabled");

        auto const* hsaco = reinterpret_cast<bin_info const*>(
            kreg::get().find("", kreg::fnv1a(""), "_HSACO_", kreg::fnv1a("_HSACO_")));
        if (hsaco) {
            DEBUG_FMT("hsaco: ptr={} len={}", fmt::ptr(hsaco->ptr), hsaco->len);
            _(hipModuleLoadData(&mod_, hsaco->ptr));
        } else {
            DEBUG_LOG("hsaco: not found");
        }

        q_task.reset(new BS::thread_pool_light(4));
    }

    void shutdown() override {
        q_task->wait_for_tasks();
        q_task.reset();

        if (auto const mod = std::exchange(mod_, nullptr)) {
            _(hipModuleUnload(mod));
        }
    }

    std::shared_ptr<rts::task> new_task() override {
        return std::make_shared<task_impl>(mod_);
    }

private:
    hipModule_t mod_ = nullptr;
};

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

std::unique_ptr<rts::subsystem> make_dev_rts_hip() {
    init_logging();
    utils::logging::timer_reset();
    dev_rts::init_time_point();
    return std::make_unique<subsystem_impl>();
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
