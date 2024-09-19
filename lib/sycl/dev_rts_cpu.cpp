#include <array>
#include <cassert>
#include <cstring>
#include "dev_rts.hpp"
#include "fiber.hpp"
#include "format.hpp"
#include "logging.hpp"

namespace {

LOGGING_DEFINE_SCOPE(cpu)

using namespace dev_rts;

struct device_impl final : device_base {
    std::string info_name() const override {
        return "Dev-RTS Device [CPU]";
    }
};

struct platform_impl final : platform_base<device_impl> {
    std::string info_name() const override {
        return "Dev-RTS Platform [CPU]";
    }
};

struct event_node_impl : event_node<event_node_impl> {};

using event_ptr = event_node_impl::event_ptr;

using weak_event_ptr = event_node_impl::weak_event_ptr;

struct event_impl final : event_base<event_node_impl> {
    using event_base::event_base;

    auto get() const {
        return ev_;
    }
};

struct buffer_impl final : buffer_base {
    explicit buffer_impl(void* h_ptr, size_t element_size, rts::range const& size)
        : buffer_base(h_ptr, element_size, size) {}

    ~buffer_impl() override {}

    void* get_pointer() override {
        return get_host_pointer();
    }

    void* get() {
        return get_host_pointer();
    }
};

struct task_impl : rts::task,
                   std::enable_shared_from_this<task_impl>,
                   private task_parameter_storage {
    task_impl() : ev_(event_node_impl::create()), wk_(ev_) {
        DEBUG_FMT("task ctor: {}", format::ptr(this));
    }

    void enable_profiling() override {
        DEBUG_FMT("start: enable_profiling: task[{}]", format::ptr(this));
        ev_->enable_profiling();
    }

    void depends_on(rts::event const& ev) override {
        static_cast<event_impl const&>(ev).get()->happens_before(ev_);
    }

    void depends_on(std::shared_ptr<rts::task> const& dep) override {
        assert(dep != nullptr);

        auto dep_ = static_cast<task_impl const&>(*dep);

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
        auto& buf_ = static_cast<buffer_impl&>(buf);
        auto const htod =
            (ma != rts::memory_access::write_only) && is_device_task_ && dom.is_host();
        auto const dtoh =
            (ma != rts::memory_access::write_only) && !is_device_task_ && !dom.is_host();

        (void)offset;
        DEBUG_FMT(
            "set_buffer_param(h_ptr={}, dom={}, ma={}, off=[{}, {}, {}], off_byte={}) htod={} "
            "dtoh={}",
            format::ptr(h_ptr), dom.id(), static_cast<int>(ma), offset.size[0], offset.size[1],
            offset.size[2], offset_byte, htod, dtoh);

        if (htod) {
            pre_ = [h_ptr, ptr = buf_.get(), length = buf_.byte_size(),
                    prev = std::move(pre_)] {
                if (prev) {
                    prev();
                }

                DEBUG_FMT("memcpy({}, {}, {}) H-to-D", format::ptr(ptr), format::ptr(h_ptr),
                          length);

                std::memcpy(ptr, h_ptr, length);
            };
        } else if (dtoh) {
            pre_ = [h_ptr, ptr = buf_.get(), length = buf_.byte_size(),
                    prev = std::move(pre_)] {
                if (prev) {
                    prev();
                }

                DEBUG_FMT("memcpy({}, {}, {}) D-to-H", format::ptr(h_ptr), format::ptr(ptr),
                          length);

                std::memcpy(h_ptr, ptr, length);
            };
        }

        auto* ptr = next_param_ptr<void*>();
        *ptr = dev_rts::advance_ptr(buf_.get(), offset_byte);
    }

    static inline void* get_ptr(rts::buffer& buf, size_t off_byte) {
        return dev_rts::advance_ptr(buf.get_pointer(), off_byte);
    }

    void copy_1d_impl(void const* src_ptr, void* dst_ptr, size_t len_byte) {
        pre_ = [src_ptr, dst_ptr, len_byte, prev = std::move(pre_)] {
            if (prev) {
                prev();
            }

            DEBUG_FMT("copy_1d(src={}, dst={}, len_byte={})", format::ptr(src_ptr),
                      format::ptr(dst_ptr), len_byte);

            std::memcpy(dst_ptr, src_ptr, len_byte);
        };
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        copy_1d_impl(get_ptr(src, src_off_byte), get_ptr(dst, dst_off_byte), len_byte);
    }

    void copy_1d(rts::buffer& src, size_t src_off_byte, void* dst, size_t len_byte) override {
        copy_1d_impl(get_ptr(src, src_off_byte), dst, len_byte);
    }

    void copy_1d(void const* src, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override {
        copy_1d_impl(src, get_ptr(dst, dst_off_byte), len_byte);
    }

    void copy_2d_impl(void const* src_ptr, size_t src_stride, void* dst_ptr, size_t dst_stride,
                      size_t loop, size_t len_byte) {
        pre_ = [src_ptr, dst_ptr, src_stride, dst_stride, loop, len_byte,
                prev = std::move(pre_)]() mutable {
            if (prev) {
                prev();
            }

            DEBUG_FMT(
                "copy_2d(src={}, dst={}, loop={}, src_stride={}, dst_stride={}, len_byte={})",
                format::ptr(src_ptr), format::ptr(dst_ptr), loop, src_stride, dst_stride,
                len_byte);

            for (size_t i = 0; i < loop; i++) {
                std::memcpy(dst_ptr, src_ptr, len_byte);

                src_ptr = dev_rts::advance_ptr(src_ptr, src_stride);
                dst_ptr = dev_rts::advance_ptr(dst_ptr, dst_stride);
            }
        };
    }

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, rts::buffer& dst,
                 size_t dst_off_byte, size_t dst_stride, size_t loop,
                 size_t len_byte) override {
        copy_2d_impl(get_ptr(src, src_off_byte), src_stride, get_ptr(dst, dst_off_byte),
                     dst_stride, loop, len_byte);
    }

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, void* dst,
                 size_t dst_stride, size_t loop, size_t len_byte) override {
        copy_2d_impl(get_ptr(src, src_off_byte), src_stride, dst, dst_stride, loop, len_byte);
    }

    void copy_2d(void const* src, size_t src_stride, rts::buffer& dst, size_t dst_off_byte,
                 size_t dst_stride, size_t loop, size_t len_byte) override {
        copy_2d_impl(src, src_stride, get_ptr(dst, dst_off_byte), dst_stride, loop, len_byte);
    }

    void copy_3d_impl(void const* src_ptr, size_t i_src_stride, size_t j_src_stride,
                      void* dst_ptr, size_t i_dst_stride, size_t j_dst_stride, size_t i_loop,
                      size_t j_loop, size_t len_byte) {
        pre_ = [src_ptr, dst_ptr, i_src_stride, j_src_stride, i_dst_stride, j_dst_stride,
                i_loop, j_loop, len_byte, prev = std::move(pre_)]() mutable {
            if (prev) {
                prev();
            }

            DEBUG_FMT(
                "copy_3d(src={}, dst={}, loop=[{}, {}], src_stride=[{}, {}], dst_stride=[{}, "
                "{}], len_byte={})",
                format::ptr(src_ptr), format::ptr(dst_ptr), i_loop, j_loop, i_src_stride,
                j_src_stride, i_dst_stride, j_dst_stride, len_byte);

            for (size_t i = 0; i < i_loop; i++) {
                auto const src_ptr0 = src_ptr;
                auto const dst_ptr0 = dst_ptr;

                for (size_t j = 0; j < j_loop; j++) {
                    std::memcpy(dst_ptr, src_ptr, len_byte);
                    src_ptr = dev_rts::advance_ptr(src_ptr, j_src_stride);
                    dst_ptr = dev_rts::advance_ptr(dst_ptr, j_dst_stride);
                }

                src_ptr = dev_rts::advance_ptr(src_ptr0, i_src_stride);
                dst_ptr = dev_rts::advance_ptr(dst_ptr0, i_dst_stride);
            }
        };
    }

    void copy_3d(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                 size_t j_src_stride, rts::buffer& dst, size_t dst_off_byte,
                 size_t i_dst_stride, size_t j_dst_stride, size_t i_loop, size_t j_loop,
                 size_t len_byte) override {
        copy_3d_impl(get_ptr(src, src_off_byte), i_src_stride, j_src_stride,
                     get_ptr(dst, dst_off_byte), i_dst_stride, j_dst_stride, i_loop, j_loop,
                     len_byte);
    }

    void copy_3d(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                 size_t j_src_stride, void* dst, size_t i_dst_stride, size_t j_dst_stride,
                 size_t i_loop, size_t j_loop, size_t len_byte) override {
        copy_3d_impl(get_ptr(src, src_off_byte), i_src_stride, j_src_stride, dst, i_dst_stride,
                     j_dst_stride, i_loop, j_loop, len_byte);
    }

    void copy_3d(void const* src, size_t i_src_stride, size_t j_src_stride, rts::buffer& dst,
                 size_t dst_off_byte, size_t i_dst_stride, size_t j_dst_stride, size_t i_loop,
                 size_t j_loop, size_t len_byte) override {
        copy_3d_impl(src, i_src_stride, j_src_stride, get_ptr(dst, dst_off_byte), i_dst_stride,
                     j_dst_stride, i_loop, j_loop, len_byte);
    }

    void fill(rts::buffer& dst, size_t byte_len) override {
        pre_ = [ptr = get_ptr(dst, 0), byte_len, prev = std::move(pre_)]() {
            if (prev) {
                prev();
            }
            memset(ptr, 0x00, byte_len);
        };
    }

    void set_kernel(char const* name, uint32_t hash) override {
        auto& reg = kreg::get();

        if (auto const* info = reg.find(name, hash, "cpu-openmp", kreg::fnv1a("cpu-openmp"))) {
            fn_ = reinterpret_cast<dev_fn_ptr_t>(info->fn);
        } else if (auto const* info = reg.find(name, hash, "cpu-c", kreg::fnv1a("cpu-c"))) {
            fn_ = reinterpret_cast<dev_fn_ptr_t>(info->fn);
        } else {
            auto errmsg = format::format("Kernel not found: {}", name);
            throw std::runtime_error(errmsg);
        }
    }

    void set_desc(rts::func_desc const* desc) override {
        desc_ = desc;
    }

    void set_single() override {
        DEBUG_LOG("set_single()");

        par_[0] = 1;
        par_[1] = 1;
        par_[2] = 1;
    }

    void set_range(rts::range const& r) override {
        DEBUG_FMT("set_range({}, {}, {})", r.size[0], r.size[1], r.size[2]);

        par_[0] = r.size[0];
        par_[1] = r.size[1];
        par_[2] = r.size[2];
    }

    void set_nd_range(rts::nd_range const& ndr) override {
        DEBUG_FMT("set_nd_range({}, {}, {}, {}, {}, {})", ndr.global[0] / ndr.local[0],
                  ndr.global[1] / ndr.local[1], ndr.global[2] / ndr.local[2], ndr.local[0],
                  ndr.local[1], ndr.local[2]);

        par_[0] = ndr.global[0] / ndr.local[0];
        par_[1] = ndr.global[1] / ndr.local[1];
        par_[2] = ndr.global[2] / ndr.local[2];
        par_[3] = ndr.local[0];
        par_[4] = ndr.local[1];
        par_[5] = ndr.local[2];
        is_ndr_ = true;
    }

    void set_local_mem_size(size_t byte) override {
        DEBUG_FMT("set_local_mem_size({})", byte);

        lmem_ = byte;

        // *next_param_ptr<unsigned int*>() = &lmem_;
    }

    void use_device() override {
        is_device_task_ = true;
    }

    void use_host() override {
        is_device_task_ = false;
    }

    void set_device([[maybe_unused]] rts::device& dev) override {
        assert(static_cast<device_impl*>(&dev) != nullptr);
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
            ev_->set_fn([task = shared_from_this()](event_ptr const& ev) mutable {
                DEBUG_FMT("start: task [{}]", format::ptr(task.get()));

                if (task->pre_) {
                    task->pre_();
                }
                if (task->body_) {
                    task->body_();
                }

                DEBUG_FMT("end:   task [{}]", format::ptr(task.get()));

                ev->complete();
            });
        }
        ev_->finalize();

        return std::make_unique<event_impl>(std::move(ev_));
    }

    void prep_device() {
        if (fn_) {
            body_ = [this]() {
                DEBUG_FMT("start: kernel function [{}]", format::ptr(this));

                if (is_ndr_) {
                    sycl::runtime::impl::exec_with_fibers(par_[0], par_[1], par_[2], par_[3],
                                                          par_[4], par_[5], lmem_, fn_,
                                                          args_.data());
                } else {
                    this->fn_(this->args_.data());
                }

                DEBUG_FMT("end:   kernel function [{}]", format::ptr(this));
            };
        }
    }

    void prep_host() {
        if (host_fn_) {
            body_ = [this]() {
                DEBUG_FMT("start: host function [{}]", format::ptr(this));

                this->host_fn_();

                DEBUG_FMT("end:   host function [{}]", format::ptr(this));
            };
        }
    }

    void prep_desc() {
        if (desc_) {
            body_ = [this]() {
                DEBUG_FMT("start: kernel descriptor {} [{}]", desc_->name, format::ptr(this));

                desc_->cpu(this->args_.data());

                DEBUG_FMT("end:   kernel descriptor {} [{}]", desc_->name, format::ptr(this));
            };
        }
    }

    using dev_fn_t = void(void**);
    using dev_fn_ptr_t = std::add_pointer_t<dev_fn_t>;

    bool is_device_task_ = true;
    std::function<dev_fn_t> fn_;
    std::function<void()> host_fn_;
    std::function<void()> pre_;
    std::function<void()> body_;
    std::array<size_t, 6> par_;
    event_ptr ev_;
    weak_event_ptr wk_;
    unsigned int lmem_ = 0;
    bool is_ndr_ = false;
    rts::func_desc const* desc_ = nullptr;
};

struct subsystem_impl final : subsystem_base<platform_impl, buffer_impl> {
    subsystem_impl() {
        init_logging();

        q_task.reset(new BS::thread_pool(1));

        DEBUG_LOG("initialized");
    }

    std::shared_ptr<rts::task> new_task() override {
        return std::make_shared<task_impl>();
    }

    void shutdown() override {
        q_task->wait();
    }
};

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

std::unique_ptr<rts::subsystem> make_dev_rts() {
    init_logging();
    logging::timer_reset();
    dev_rts::init_time_point();
    fiber_init();
    return std::make_unique<subsystem_impl>();
}

}  // namespace runtime::impl

CHARM_SYCL_END_NAMESPACE
