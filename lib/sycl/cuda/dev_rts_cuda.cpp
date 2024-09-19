#include <array>
#include <cassert>
#include <cstring>
#include <variant>
#include <blas/cublas_interface.hpp>
#include <blas/cusolver_interface.hpp>
#include <cuda/cuda_interface.hpp>
#include "../dev_rts.hpp"
#include "../interfaces.hpp"
#include "../logging.hpp"
#include "context.hpp"
#include "dev_rts/coarse_task.hpp"

using CUDA = sycl::runtime::cuda_interface;
using BLAS = sycl::runtime::cublas_interface_11000;
using SOL = sycl::runtime::cusolver_interface_11000;

namespace {

struct env_flag {
    env_flag(char const* name) {
        if (auto const* opt = getenv(name)) {
            val_ = strcasecmp(opt, "1") == 0 || strcasecmp(opt, "y") == 0 ||
                   strcasecmp(opt, "yes") == 0 || strcasecmp(opt, "t") == 0 ||
                   strcasecmp(opt, "true") == 0;
        }
    }

    void set(bool val) {
        val_ = val;
    }

    bool get() const {
        return val_;
    }

    operator bool() const {
        return get();
    }

private:
    bool val_ = false;
};

auto const OPT_PIN = env_flag("CHARM_SYCL_CUDA_PIN");

namespace dev_rts = sycl::dev_rts;
namespace rts = sycl::rts;

LOGGING_DEFINE_SCOPE(cuda)

template <class CUDA>
inline void check_cuda_error(typename CUDA::result_t err, char const* expr) {
    if (err != CUDA::k_CUDA_SUCCESS) {
        std::string errmsg;
        errmsg = "CUDA Error: ";

        char const* err_str;
        CUDA::cu_get_error_string(err, &err_str);
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

#define _(expr) check_cuda_error<CUDA>((expr), #expr)

template <class CUDA>
struct queue {
    queue() : q_(1) {}

    void init() {
        _(CUDA::cu_stream_create(&stream_, 0));

        q_.submit_task([] {}).wait();
    }

    template <class F>
    void push(F&& fn) {
        q_.detach_task([this, fn = std::forward<F>(fn)] {
            fn(stream_);
        });
    }

    void wait() {
        q_.wait();
    }

private:
    typename CUDA::stream_t stream_;
    BS::thread_pool q_;
};

template <class CUDA>
struct queues {
    inline static queue<CUDA> h2d, d2h, d2d, fill, kernel, host;

    static void init_all() {
        h2d.init();
        d2h.init();
        d2d.init();
        fill.init();
        kernel.init();
        host.init();
    }

    static void wait_all() {
        h2d.wait();
        d2h.wait();
        d2d.wait();
        fill.wait();
        kernel.wait();
        host.wait();
    }
};

struct device_impl final : ::dev_rts::device_base {
    std::string info_name() const override {
        return "Dev-RTS Device [CUDA]";
    }
};

struct platform_impl final : ::dev_rts::platform_base<device_impl> {
    std::string info_name() const override {
        return "Dev-RTS Platform [CUDA]";
    }
};

template <class CUDA>
struct buffer_impl final : ::dev_rts::buffer_base {
    using deviceptr_t = typename CUDA::deviceptr_t;

    explicit buffer_impl(void* h_ptr, size_t element_size, rts::range const& size)
        : buffer_base(h_ptr, element_size, size) {
        _(CUDA::cu_mem_alloc(&devptr_, byte_size()));

        auto const pinned = OPT_PIN && h_ptr;

        DEBUG_FMT("this={} {}(size={}) => 0x{:x}{}", format::ptr(this), __func__, byte_size(),
                  devptr_.get(), pinned ? " (pinned)" : "");

        if (pinned) {
            _(CUDA::cu_mem_host_register(h_ptr, byte_size(), 0));
        }
    }

    ~buffer_impl() override {
        auto* h_ptr = get_host_pointer();
        auto const pinned = OPT_PIN && h_ptr;

        DEBUG_FMT("this={} {}(devptr={}){}", format::ptr(this), __func__, devptr_.get(),
                  pinned ? " (unpin)" : "");

        if (pinned) {
            _(CUDA::cu_mem_host_unregister(h_ptr));
        }

        _(CUDA::cu_mem_free(devptr_));
        devptr_ = {};
    }

    void* get_pointer() override {
        return reinterpret_cast<void*>(*devptr_);
    }

    deviceptr_t get() {
        return devptr_;
    }

private:
    deviceptr_t devptr_;
};

template <class CUDA>
struct memcpy1d {
    template <class S, class D>
    explicit memcpy1d(S&& s, D&& d, size_t l)
        : src(std::forward<S>(s)), dst(std::forward<D>(d)), len(l) {}

    std::variant<typename CUDA::deviceptr_t, void const*> src;
    std::variant<typename CUDA::deviceptr_t, void*> dst;
    size_t len;
};

template <class CUDA>
struct memcpy_op : dev_rts::op_base {
    using deviceptr_t = typename CUDA::deviceptr_t;
    using memcpy1d_t = memcpy1d<CUDA>;
    using memcpy2d_t = typename CUDA::memcpy2d_t;
    using memcpy3d_t = typename CUDA::memcpy3d_t;
    using stream_t = typename CUDA::stream_t;
    using queues_t = queues<CUDA>;
    using memorytype_t = typename CUDA::memorytype_t;

    template <class S, class D>
    explicit memcpy_op(S&& src, D&& dst, size_t len)
        : desc_(memcpy1d_t(std::forward<S>(src), std::forward<D>(dst), len)) {}

    explicit memcpy_op(memcpy2d_t const& desc) : desc_(desc) {}

    explicit memcpy_op(memcpy3d_t const& desc) : desc_(desc) {}

    void call(dev_rts::task_ptr const& task) override {
        std::visit(
            [&](auto&& v1) {
                (*this)(std::forward<decltype(v1)>(v1), task);
            },
            desc_);
    }

    void operator()(memcpy1d_t const& copy, dev_rts::task_ptr const& task) {
        std::visit(
            [&](auto&& v1, auto&& v2) {
                (*this)(std::forward<decltype(v1)>(v1), std::forward<decltype(v2)>(v2), task);
            },
            copy.dst, copy.src);
    }

    void operator()(memcpy2d_t const& copy, dev_rts::task_ptr const& task) {
        (*this)(CUDA::get_srcMemoryType(copy), CUDA::get_dstMemoryType(copy), task);
    }

    void operator()(memcpy3d_t const& copy, dev_rts::task_ptr const& task) {
        (*this)(CUDA::get_srcMemoryType(copy), CUDA::get_dstMemoryType(copy), task);
    }

    void operator()(memorytype_t s_type, memorytype_t d_type, dev_rts::task_ptr const& task) {
        if (s_type == CUDA::k_MEMORYTYPE_DEVICE && d_type == CUDA::k_MEMORYTYPE_DEVICE) {
            queues_t::d2d.push([task, this](stream_t stream) {
                (*this)(stream);
                _(CUDA::cu_stream_synchronize(stream));

                DEBUG_FMT("this={} task={} complete", format::ptr(this),
                          format::ptr(task.get()));
                task->complete();
            });
        } else if (s_type == CUDA::k_MEMORYTYPE_DEVICE) {
            queues_t::d2h.push([task, this](stream_t stream) {
                (*this)(stream);
                _(CUDA::cu_stream_synchronize(stream));

                DEBUG_FMT("this={} task={} complete", format::ptr(this),
                          format::ptr(task.get()));
                task->complete();
            });
        } else {
            queues_t::h2d.push([task, this](stream_t stream) {
                (*this)(stream);
                _(CUDA::cu_stream_synchronize(stream));

                DEBUG_FMT("this={} task={} complete", format::ptr(this),
                          format::ptr(task.get()));
                task->complete();
            });
        }
    }

    void operator()(deviceptr_t, deviceptr_t, dev_rts::task_ptr const& task) {
        (*this)(CUDA::k_MEMORYTYPE_DEVICE, CUDA::k_MEMORYTYPE_DEVICE, task);
    }

    void operator()(void*, deviceptr_t, dev_rts::task_ptr const& task) {
        (*this)(CUDA::k_MEMORYTYPE_DEVICE, CUDA::k_MEMORYTYPE_HOST, task);
    }

    void operator()(deviceptr_t, void const*, dev_rts::task_ptr const& task) {
        (*this)(CUDA::k_MEMORYTYPE_HOST, CUDA::k_MEMORYTYPE_DEVICE, task);
    }

    void operator()(void*, const void*, dev_rts::task_ptr const&) {
        std::abort();
    }

    void operator()(stream_t const& stream) {
        std::visit(
            [&](auto&& v1) {
                (*this)(std::forward<decltype(v1)>(v1), stream);
            },
            desc_);
    }

    void operator()(memcpy1d_t const& copy, stream_t const& stream) {
        std::visit(
            [&](auto&& v1, auto&& v2) {
                (*this)(std::forward<decltype(v1)>(v1), std::forward<decltype(v2)>(v2),
                        copy.len, stream);
            },
            copy.dst, copy.src);
    }

    void operator()(deviceptr_t dst, deviceptr_t src, size_t length, stream_t const& stream) {
        DEBUG_FMT("cuMemcpyDtoDAsync(this={}, src=0x{:x}, dst=0x{:x}, length={}, stream={})",
                  format::ptr(this), src.get(), dst.get(), length, format::ptr(stream.get()));
        _(CUDA::cu_memcpy_dtod_async(dst, src, length, stream));
    }

    void operator()(deviceptr_t dst, void const* src, size_t length, stream_t const& stream) {
        DEBUG_FMT("cuMemcpyHtoDAsync(this={}, src={}, dst=0x{:x}, length={}, stream={})",
                  format::ptr(this), src, dst.get(), length, format::ptr(stream.get()));
        _(CUDA::cu_memcpy_htod_async(dst, src, length, stream));
    }

    void operator()(void* dst, deviceptr_t src, size_t length, stream_t const& stream) {
        DEBUG_FMT("cuMemcpyDtoHAsync(this={}, src=0x{:x}, dst={}, length={}, stream={})",
                  format::ptr(this), src.get(), dst, length, format::ptr(stream.get()));
        _(CUDA::cu_memcpy_dtoh_async(dst, src, length, stream));
    }

    void operator()(void*, void const*, size_t, stream_t const&) {
        std::abort();
    }

    void operator()(memcpy2d_t const& desc, stream_t const& stream) {
        DEBUG_FMT("cuMemcpy2DAsync(stream={})", format::ptr(stream.get()));
        _(CUDA::cu_memcpy2d_async(&desc, stream));
    }

    void operator()(memcpy3d_t const& desc, stream_t const& stream) {
        DEBUG_FMT("cuMemcpy3DAsync(stream={})", format::ptr(stream.get()));
        _(CUDA::cu_memcpy3d_async(&desc, stream));
    }

    std::variant<memcpy1d_t, memcpy2d_t, memcpy3d_t> desc_;
};

template <class CUDA>
struct fill_op : dev_rts::op_base {
    using deviceptr_t = typename CUDA::deviceptr_t;
    using stream_t = typename CUDA::stream_t;

    explicit fill_op(deviceptr_t dst, size_t byte_len) : dst_(dst), byte_len_(byte_len) {}

    void call(dev_rts::task_ptr const& task) override {
        queues<CUDA>::fill.push([task, this](stream_t stream) {
            (*this)(stream);
            _(CUDA::cu_stream_synchronize(stream));

            DEBUG_FMT("this={} task={} complete", format::ptr(this), format::ptr(task.get()));
            task->complete();
        });
    }

    void operator()(stream_t stream) {
        DEBUG_FMT("this={} cuMemsetD8Async(dst=0x{:x}, 0x00, byte_len={}, stream={})",
                  format::ptr(this), dst_.get(), byte_len_, format::ptr(stream.get()));
        _(CUDA::cu_memset_d8_async(dst_, 0x00, byte_len_, stream));
    }

private:
    deviceptr_t dst_;
    size_t byte_len_;
};

template <class CUDA>
struct kernel_op : dev_rts::op_base {
    using module_t = typename CUDA::module_t;
    using function_t = typename CUDA::function_t;
    using stream_t = typename CUDA::stream_t;

    explicit kernel_op(std::shared_ptr<dev_rts::coarse_task::kernel_desc> const& desc,
                       module_t mod, int n_sm)
        : desc_(desc), mod_(mod), n_sm_(n_sm) {}

    void call(dev_rts::task_ptr const& task) override {
        if (desc_->host_fn) {
            queues<CUDA>::host.push([task, this](stream_t stream) {
                (*this)(stream);

                DEBUG_FMT("this={} task={} complete", format::ptr(this),
                          format::ptr(task.get()));
                task->complete();
            });
        } else if (desc_->name || desc_->desc) {
            queues<CUDA>::kernel.push([task, this](stream_t stream) {
                (*this)(stream);
                _(CUDA::cu_stream_synchronize(stream));

                DEBUG_FMT("this={} task={} complete", format::ptr(this),
                          format::ptr(task.get()));
                task->complete();
            });
        } else {
            DEBUG_FMT("this={} task={} complete", format::ptr(this), format::ptr(task.get()));
            task->complete();
        }
    }

    void operator()(stream_t stream) {
        if (auto& fn = desc_->host_fn) {
            fn();
            return;
        }

        function_t fn;
        _(CUDA::cu_module_get_function(&fn, mod_, desc_->name));

        if (desc_->is_ndr) {
            auto const gz = desc_->range[0];
            auto const gy = desc_->range[1];
            auto const gx = desc_->range[2];
            auto const bz = desc_->range[3];
            auto const by = desc_->range[4];
            auto const bx = desc_->range[5];

            DEBUG_FMT("this={} LaunchKernel({}, {}, {}, {}, {}, {}, {}, {}, stream=0x{})",
                      format::ptr(this), desc_->name, gx, gy, gz, bx, by, bz, desc_->lmem,
                      format::ptr(*stream));
            _(CUDA::cu_launch_kernel(fn, gx, gy, gz, bx, by, bz, desc_->lmem, stream,
                                     desc_->args.data(), nullptr));
        } else {
            unsigned gx, gy, gz, bx, by, bz;
            heuristic(gx, gy, gz, bx, by, bz);

            DEBUG_FMT("this={} LaunchKernel({}, {}, {}, {}, {}, {}, {}, {}, stream=0x{})",
                      format::ptr(this), desc_->name, gx, gy, gz, bx, by, bz, desc_->lmem,
                      format::ptr(*stream));

            _(CUDA::cu_launch_kernel(fn, gx, gy, gz, bx, by, bz, desc_->lmem, stream,
                                     desc_->args.data(), nullptr));
        }
    }

private:
    void heuristic(unsigned& gx, unsigned& gy, unsigned& gz, unsigned& bx, unsigned& by,
                   unsigned& bz) const {
        gx = 1;
        bx = 1;
        gy = desc_->range[1];
        by = 1;
        gz = desc_->range[0];
        bz = 1;

        if (desc_->is_single) {
            return;
        }

        if (desc_->range[0] == 1 && desc_->range[1] == 1) {
            auto const n = desc_->range[2];
            bx = 512u;
            gx = std::min<unsigned int>(n_sm_ * 8, (n + bx - 1) / bx);
        } else {
            auto const n = desc_->range[2];
            bx = std::min<unsigned int>(n, 512);
            gx = (n + bx - 1) / bx;
        }
    }

    std::shared_ptr<dev_rts::coarse_task::kernel_desc> desc_;
    module_t mod_;
    int n_sm_;
};

template <class CUDA, class BLAS, class SOL>
struct task_impl final : dev_rts::coarse_task {
    using buffer_t = buffer_impl<CUDA>;
    using deviceptr_t = typename CUDA::deviceptr_t;
    using function_t = typename CUDA::function_t;
    using memcpy2d_t = typename CUDA::memcpy2d_t;
    using memcpy3d_t = typename CUDA::memcpy3d_t;
    using module_t = typename CUDA::module_t;
    using stream_t = typename CUDA::stream_t;
    using copy_t = memcpy_op<CUDA>;
    using fill_t = fill_op<CUDA>;
    using kernel_t = kernel_op<CUDA>;

    explicit task_impl(module_t mod, int n_sm) : mod_(mod), n_sm_(n_sm) {}

    ~task_impl() = default;

private:
    static inline deviceptr_t get_ptr(rts::buffer& buf, size_t off_byte) {
        return static_cast<buffer_t&>(buf).get() + deviceptr_t(off_byte);
    }

    void set_buffer_param(rts::buffer& buf, void* h_ptr, rts::memory_domain const& dom,
                          rts::memory_access ma, rts::id const&, size_t offset_byte) override {
        auto& buf_ = static_cast<buffer_t&>(buf);
        auto const is_device_task = k_.is_device;
        auto const htod =
            (ma != rts::memory_access::write_only) && is_device_task && dom.is_host();
        auto const dtoh =
            (ma != rts::memory_access::write_only) && !is_device_task && !dom.is_host();

        DEBUG_FMT("this={} {}(htod={}, dtoh={})", format::ptr(this), __func__, htod, dtoh);

        if (htod) {
            commit(make_copy_1d_op(h_ptr, buf_, 0, buf_.byte_size()));
        } else if (dtoh) {
            commit(make_copy_1d_op(buf_, 0, h_ptr, buf_.byte_size()));
        } else {
            commit();
        }

        auto const devptr = get_ptr(buf, offset_byte);
        k_.add_param(devptr);
    }

    dev_rts::op_ptr make_kernel_op() override {
        return std::make_unique<kernel_t>(kdesc(), mod_, n_sm_);
    }

    dev_rts::op_ptr make_copy_1d_op(rts::buffer& src, size_t src_off_byte, rts::buffer& dst,
                                    size_t dst_off_byte, size_t len_byte) override {
        return std::make_unique<copy_t>(get_ptr(src, src_off_byte), get_ptr(dst, dst_off_byte),
                                        len_byte);
    }

    dev_rts::op_ptr make_copy_1d_op(rts::buffer& src, size_t src_off_byte, void* dst,
                                    size_t len_byte) override {
        return std::make_unique<copy_t>(get_ptr(src, src_off_byte), dst, len_byte);
    }

    dev_rts::op_ptr make_copy_1d_op(void const* src, rts::buffer& dst, size_t dst_off_byte,
                                    size_t len_byte) override {
        return std::make_unique<copy_t>(src, get_ptr(dst, dst_off_byte), len_byte);
    }

    dev_rts::op_ptr make_copy_2d_op(rts::buffer& src, size_t src_off_byte, size_t src_stride,
                                    rts::buffer& dst, size_t dst_off_byte, size_t dst_stride,
                                    size_t loop, size_t len_byte) override {
        memcpy2d_t desc;

        CUDA::set_srcMemoryType(desc, CUDA::k_MEMORYTYPE_DEVICE);
        CUDA::set_srcDevice(desc, get_ptr(src, src_off_byte));
        CUDA::set_srcPitch(desc, src_stride);
        CUDA::set_dstMemoryType(desc, CUDA::k_MEMORYTYPE_DEVICE);
        CUDA::set_dstDevice(desc, get_ptr(dst, dst_off_byte));
        CUDA::set_dstPitch(desc, dst_stride);
        CUDA::set_WidthInBytes(desc, len_byte);
        CUDA::set_Height(desc, loop);

        return std::make_unique<copy_t>(desc);
    }

    dev_rts::op_ptr make_copy_2d_op(rts::buffer& src, size_t src_off_byte, size_t src_stride,
                                    void* dst, size_t dst_stride, size_t loop,
                                    size_t len_byte) override {
        memcpy2d_t desc;

        CUDA::set_srcMemoryType(desc, CUDA::k_MEMORYTYPE_DEVICE);
        CUDA::set_srcDevice(desc, get_ptr(src, src_off_byte));
        CUDA::set_srcPitch(desc, src_stride);
        CUDA::set_dstMemoryType(desc, CUDA::k_MEMORYTYPE_HOST);
        CUDA::set_dstHost(desc, dst);
        CUDA::set_dstPitch(desc, dst_stride);
        CUDA::set_WidthInBytes(desc, len_byte);
        CUDA::set_Height(desc, loop);

        return std::make_unique<copy_t>(desc);
    }

    dev_rts::op_ptr make_copy_2d_op(void const* src, size_t src_stride, rts::buffer& dst,
                                    size_t dst_off_byte, size_t dst_stride, size_t loop,
                                    size_t len_byte) override {
        memcpy2d_t desc;

        CUDA::set_srcMemoryType(desc, CUDA::k_MEMORYTYPE_HOST);
        CUDA::set_srcHost(desc, src);
        CUDA::set_srcPitch(desc, src_stride);
        CUDA::set_dstMemoryType(desc, CUDA::k_MEMORYTYPE_DEVICE);
        CUDA::set_dstDevice(desc, get_ptr(dst, dst_off_byte));
        CUDA::set_dstPitch(desc, dst_stride);
        CUDA::set_WidthInBytes(desc, len_byte);
        CUDA::set_Height(desc, loop);

        return std::make_unique<copy_t>(desc);
    }

    dev_rts::op_ptr make_copy_3d_op(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                                    size_t j_src_stride, rts::buffer& dst, size_t dst_off_byte,
                                    size_t i_dst_stride, size_t j_dst_stride, size_t i_loop,
                                    size_t j_loop, size_t len_byte) override {
        if (i_src_stride % j_src_stride != 0) {
            std::terminate();
        }
        if (i_dst_stride % j_dst_stride != 0) {
            std::terminate();
        }

        memcpy3d_t desc;

        CUDA::set_srcMemoryType(desc, CUDA::k_MEMORYTYPE_DEVICE);
        CUDA::set_srcDevice(desc, get_ptr(src, src_off_byte));
        CUDA::set_srcPitch(desc, j_src_stride);
        CUDA::set_srcHeight(desc, i_src_stride / j_src_stride);
        CUDA::set_dstMemoryType(desc, CUDA::k_MEMORYTYPE_DEVICE);
        CUDA::set_dstDevice(desc, get_ptr(dst, dst_off_byte));
        CUDA::set_dstPitch(desc, j_dst_stride);
        CUDA::set_dstHeight(desc, i_dst_stride / j_dst_stride);
        CUDA::set_WidthInBytes(desc, len_byte);
        CUDA::set_Height(desc, j_loop);
        CUDA::set_Depth(desc, i_loop);

        return std::make_unique<copy_t>(desc);
    }

    dev_rts::op_ptr make_copy_3d_op(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                                    size_t j_src_stride, void* dst, size_t i_dst_stride,
                                    size_t j_dst_stride, size_t i_loop, size_t j_loop,
                                    size_t len_byte) override {
        if (i_src_stride % j_src_stride != 0) {
            std::terminate();
        }
        if (i_dst_stride % j_dst_stride != 0) {
            std::terminate();
        }

        memcpy3d_t desc;

        CUDA::set_srcMemoryType(desc, CUDA::k_MEMORYTYPE_DEVICE);
        CUDA::set_srcDevice(desc, get_ptr(src, src_off_byte));
        CUDA::set_srcPitch(desc, j_src_stride);
        CUDA::set_srcHeight(desc, i_src_stride / j_src_stride);
        CUDA::set_dstMemoryType(desc, CUDA::k_MEMORYTYPE_HOST);
        CUDA::set_dstHost(desc, dst);
        CUDA::set_dstPitch(desc, j_dst_stride);
        CUDA::set_dstHeight(desc, i_dst_stride / j_dst_stride);
        CUDA::set_WidthInBytes(desc, len_byte);
        CUDA::set_Height(desc, j_loop);
        CUDA::set_Depth(desc, i_loop);

        return std::make_unique<copy_t>(desc);
    }

    dev_rts::op_ptr make_copy_3d_op(void const* src, size_t i_src_stride, size_t j_src_stride,
                                    rts::buffer& dst, size_t dst_off_byte, size_t i_dst_stride,
                                    size_t j_dst_stride, size_t i_loop, size_t j_loop,
                                    size_t len_byte) override {
        if (i_src_stride % j_src_stride != 0) {
            std::terminate();
        }
        if (i_dst_stride % j_dst_stride != 0) {
            std::terminate();
        }

        memcpy3d_t desc;

        CUDA::set_srcMemoryType(desc, CUDA::k_MEMORYTYPE_HOST);
        CUDA::set_srcHost(desc, src);
        CUDA::set_srcPitch(desc, j_src_stride);
        CUDA::set_srcHeight(desc, i_src_stride / j_src_stride);
        CUDA::set_dstMemoryType(desc, CUDA::k_MEMORYTYPE_DEVICE);
        CUDA::set_dstDevice(desc, get_ptr(dst, dst_off_byte));
        CUDA::set_dstPitch(desc, j_dst_stride);
        CUDA::set_dstHeight(desc, i_dst_stride / j_dst_stride);
        CUDA::set_WidthInBytes(desc, len_byte);
        CUDA::set_Height(desc, j_loop);
        CUDA::set_Depth(desc, i_loop);

        return std::make_unique<copy_t>(desc);
    }

    dev_rts::op_ptr make_fill_op(rts::buffer& dst, size_t byte_len) override {
        return std::make_unique<fill_t>(get_ptr(dst, 0), byte_len);
    }

private:
    module_t mod_;
    int n_sm_;
};

struct bin_info {
    char const* ptr;
    ssize_t len;
};

template <class CUDA, class BLAS, class SOL>
struct subsystem_impl final : ::dev_rts::subsystem_base<platform_impl, buffer_impl<CUDA>> {
    using result_t = typename CUDA::result_t;
    using module_t = typename CUDA::module_t;
    using device_t = typename CUDA::device_t;
    using context_t = typename CUDA::context_t;

    subsystem_impl() {
        init_logging();

        _(CUDA::cu_init(0));
        DEBUG_LOG("enabled");

        _(CUDA::cu_device_get(&dev_, 0));
        _(CUDA::cu_device_primary_ctx_set_flags(dev_, 0));
        _(CUDA::cu_device_primary_ctx_retain(&ctx_, dev_));
        _(CUDA::cu_ctx_set_current(ctx_));

        auto const* kinfo =
            kreg::get().find("", kreg::fnv1a(""), "_FATBIN_", kreg::fnv1a("_FATBIN_"));
        auto const* fatbin = reinterpret_cast<bin_info const*>(kinfo ? kinfo->fn : nullptr);
        if (fatbin) {
            DEBUG_FMT("fatbin: ptr={} len={}", format::ptr(fatbin->ptr), fatbin->len);
            _(CUDA::cu_module_load_fat_binary(&mod_, fatbin->ptr));
        } else {
            DEBUG_LOG("fatbin: not found");
        }

        _(CUDA::cuDeviceGetAttribute(&n_sm_, CUDA::k_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT,
                                     dev_));

        int constexpr n_threads = 4;
        sycl::runtime::cuda_contexts<CUDA, BLAS, SOL>::cuda = ctx_;
        sycl::runtime::cuda_contexts<CUDA, BLAS, SOL>::workspaces.resize(n_threads);
        // q_task.reset(new BS::thread_pool(n_threads));
        queues<CUDA>::init_all();
    }

    void shutdown() override {
        queues<CUDA>::wait_all();
        // q_task->wait();
        // q_task.reset();
        sycl::runtime::cuda_contexts<CUDA, BLAS, SOL>::workspaces.clear();

        result_t err1 = CUDA::k_CUDA_SUCCESS;
        if (mod_) {
            err1 = CUDA::cu_module_unload(mod_);
        }

        auto const err2 = CUDA::cu_device_primary_ctx_release(dev_);

        mod_ = {};
        dev_ = {};
        ctx_ = {};

        check_cuda_error<CUDA>(err1, "cuModuleUnload");
        check_cuda_error<CUDA>(err2, "cuDevicePrimaryCtxRelease");

        CUDA::close();
    }

    std::shared_ptr<rts::task> new_task() override {
        return std::make_shared<task_impl<CUDA, BLAS, SOL>>(mod_, n_sm_);
    }

private:
    module_t release() {
        return std::exchange(mod_, {});
    }

    module_t mod_;
    device_t dev_;
    context_t ctx_;
    int n_sm_;
};

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {
error::result<std::unique_ptr<rts::subsystem>> make_dev_rts_cuda() {
    init_logging();
    logging::timer_reset();
    ::dev_rts::init_time_point();

    CHECK_ERROR(CUDA::init());
    return std::make_unique<subsystem_impl<CUDA, BLAS, SOL>>();
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
