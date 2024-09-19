#include "dep.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <assert.h>
#include "logging.hpp"
#include "rts.hpp"

namespace {

LOGGING_DEFINE_SCOPE(dep)

namespace dep = CHARM_SYCL_NS::dep;
namespace rts = CHARM_SYCL_NS::rts;

static std::mutex g_lock;

constexpr uint64_t HOST_INIT_VER = 1;
constexpr uint64_t DEV_INIT_VER = 0;

struct delete_by_free {
    void operator()(void* p) const {
        free(p);
    }
};

struct device_impl final : dep::device {
    explicit device_impl(std::shared_ptr<dep::dependency_manager> const& mgr,
                         std::shared_ptr<rts::device>&& rts)
        : mgr_(mgr), rts_(std::move(rts)) {}

    explicit device_impl(std::shared_ptr<dep::dependency_manager> const& mgr,
                         std::shared_ptr<rts::device> const& rts)
        : mgr_(mgr), rts_(rts) {}

    std::shared_ptr<rts::device> const& get() override {
        return rts_;
    }

    dep::memory_domain const& get_memory_domain() const override {
        return rts_->get_memory_domain();
    }

    bool is_cpu() const override {
        return rts_->is_cpu();
    }

    bool is_gpu() const override {
        return rts_->is_gpu();
    }

    bool is_accelerator() const override {
        return rts_->is_accelerator();
    }

    bool is_fpga() const override {
        return rts_->is_fpga();
    }

    bool is_custom() const override {
        return rts_->is_custom();
    }

    bool is_host() const override {
        return rts_->is_host();
    }

    std::string info_name() const override {
        return rts_->info_name();
    }

    std::string info_vendor() const override {
        return rts_->info_vendor();
    }

    std::string info_driver_version() const override {
        return rts_->info_driver_version();
    }

private:
    std::shared_ptr<dep::dependency_manager> mgr_;
    std::shared_ptr<rts::device> rts_;
};

struct platform_impl final : dep::platform {
    explicit platform_impl(std::shared_ptr<dep::dependency_manager> const& mgr,
                           std::shared_ptr<rts::platform>&& rts)
        : mgr_(mgr), rts_(std::move(rts)) {}

    explicit platform_impl(std::shared_ptr<dep::dependency_manager> const& mgr,
                           std::shared_ptr<rts::platform> const& rts)
        : mgr_(mgr), rts_(rts) {}

    std::vector<std::shared_ptr<dep::device>> get_devices() const override {
        std::vector<std::shared_ptr<dep::device>> ret;

        for (auto d : rts_->get_devices()) {
            ret.push_back(std::make_shared<device_impl>(mgr_, d));
        }

        return ret;
    }

    std::string info_name() const override {
        return rts_->info_name();
    }

    std::string info_vendor() const override {
        return rts_->info_vendor();
    }

    std::string info_version() const override {
        return rts_->info_version();
    }

private:
    std::shared_ptr<dep::dependency_manager> mgr_;
    std::shared_ptr<rts::platform> rts_;
};

struct dependency_manager_impl final : std::enable_shared_from_this<dependency_manager_impl>,
                                       dep::dependency_manager {
    explicit dependency_manager_impl(std::shared_ptr<rts::subsystem> const& ss) : ss_(ss) {
        init_logging();
    }

    ~dependency_manager_impl() {
        ss_->shutdown();
    }

    std::shared_ptr<dep::task> new_task() override;

    std::unique_ptr<dep::buffer> new_buffer(void* h_ptr, size_t element_size,
                                            rts::range size) override;

    std::vector<std::shared_ptr<dep::platform>> get_platforms() override;

    void local_read(std::shared_ptr<rts::task> const& task, dep::buffer& buf,
                    dep::memory_domain const& dom);

    void local_write(std::shared_ptr<rts::task> const& task, dep::buffer& buf,
                     dep::memory_domain const& dom);

    void local_read_write(std::shared_ptr<rts::task> const& task, dep::buffer& buf,
                          dep::memory_domain const& dom);

    void transfer_read(std::shared_ptr<rts::task> const& task, dep::buffer& buf,
                       dep::memory_domain const& src, dep::memory_domain const& dst);

    void transfer_write(std::shared_ptr<rts::task> const& task, dep::buffer& buf,
                        dep::memory_domain const& src, dep::memory_domain const& dst);

    void transfer_read_write(std::shared_ptr<rts::task> const& task, dep::buffer& buf,
                             dep::memory_domain const& src, dep::memory_domain const& dst);

    dep::memory_domain& get_host_memory_domain() {
        return ss_->get_host_memory_domain();
    }

private:
    std::shared_ptr<rts::subsystem> ss_;
};

struct memory_state {
    void init(uint64_t ver) {
        ver_ = ver;
    }

    void prepare_read(std::shared_ptr<rts::task> const& task) {
        set_dependency(task, false);
        read_ = true;
        add_reader(task);
    }

    void prepare_write(std::shared_ptr<rts::task> const& task, uint64_t new_ver) {
        set_dependency(task, true);
        read_ = true;
        readers_.clear();
        set_writer(task);
        ver_ = new_ver;
    }

    uint64_t version() const {
        return ver_;
    }

private:
    void add_reader(std::shared_ptr<rts::task> const& task) {
        assert(read_);
        assert(task != nullptr);

        readers_.push_back(task);
    }

    void set_writer(std::shared_ptr<rts::task> const& task) {
        assert(read_ && readers_.empty());
        assert(task != nullptr);

        read_ = false;
        writer_ = task;
    }

    bool is_reading() const {
        return read_ && !readers_.empty();
    }

    bool is_writing() const {
        return !read_;
    }

    void clear() {
        read_ = true;
        writer_ = nullptr;
        readers_.clear();
    }

    void set_dependency(std::shared_ptr<rts::task> const& task, bool wait_for_prior_readers) {
        assert(task != nullptr);

        if (writer_) {
            DEBUG_FMT("buffer[{}] task[{}] depends on writer[{}]", format::ptr(this),
                      format::ptr(task.get()), format::ptr(writer_.get()));
            task->depends_on(writer_);
        }

        if (wait_for_prior_readers) {
            for (auto& r : readers_) {
                DEBUG_FMT("buffer[{}] task[{}] depends on reader[{}]", format::ptr(this),
                          format::ptr(task.get()), format::ptr(r.get()));
                task->depends_on(r);
            }
        }
    }

    bool read_ = true;
    std::shared_ptr<rts::task> writer_;
    std::vector<std::shared_ptr<rts::task>> readers_;
    uint64_t ver_ = DEV_INIT_VER;
};

struct memory_state_map {
    memory_state_map() : states_(1) {
        states_.front().init(HOST_INIT_VER);
    }

    memory_state& get(rts::dom_id dom) {
        if (static_cast<size_t>(dom) >= states_.size()) {
            states_.resize(dom + 1);
        }
        return states_.at(dom);
    }

    memory_state const& get(rts::dom_id dom) const {
        return const_cast<memory_state_map*>(this)->get(dom);
    }

private:
    std::vector<memory_state> states_;
};

struct buffer_impl final : dep::buffer {
    buffer_impl(std::shared_ptr<dep::dependency_manager>&& mgr, dep::memory_domain const& h_dom,
                void* h_ptr, std::unique_ptr<void, delete_by_free>&& hp,
                std::unique_ptr<rts::buffer>&& rts)
        : mgr_(std::move(mgr)),
          h_ptr_(h_ptr),
          hp_(std::move(hp)),
          ver_(HOST_INIT_VER),
          owner_(h_dom),
          rts_(std::move(rts)) {
        assert(h_dom.id() == rts::HOST_DOM_ID);
    }

    void* get_pointer() override {
        return rts_->get_pointer();
    }

    void* get_host_pointer() override {
        return h_ptr();
    }

    memory_state& get_state(rts::dom_id dom) {
        return map_.get(dom);
    }

    memory_state const& get_state(rts::dom_id dom) const {
        return map_.get(dom);
    }

    bool has_latest(rts::dom_id dom) const {
        return version() == get_state(dom).version();
    }

    uint64_t version() const {
        return ver_;
    }

    dep::memory_domain const& owner() const {
        return owner_;
    }

    void set_version(dep::memory_domain const& new_owner, uint64_t new_ver) {
        assert(ver_ < new_ver);
        owner_ = new_owner;
        ver_ = new_ver;
    }

    rts::buffer& to_rts() {
        return *rts_;
    }

    void* h_ptr() {
        return h_ptr_;
    }

private:
    std::shared_ptr<dep::dependency_manager> mgr_;
    void* h_ptr_;
    std::unique_ptr<void, delete_by_free> hp_;
    uint64_t ver_;
    std::reference_wrapper<dep::memory_domain const> owner_;
    memory_state_map map_;
    std::unique_ptr<rts::buffer> rts_;
};

struct task_impl final : dep::task {
    explicit task_impl(dependency_manager_impl& dep, std::shared_ptr<rts::task> rts)
        : dep_(dep), rts_(rts) {
        DEBUG_FMT("task[{}] create (this={})", format::ptr(rts_.get()), format::ptr(this));
    }

    ~task_impl() {
        DEBUG_FMT("task[{}] destroy (this={})", format::ptr(rts_.get()), format::ptr(this));
    }

    void enable_profiling() override {
        rts_->enable_profiling();
    }

    void depends_on(dep::event const& ev) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));

        rts_->depends_on(ev);
    }

    void depends_on(std::shared_ptr<dep::task> const& task) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));

        auto task_ = std::dynamic_pointer_cast<task_impl>(task);
        rts_->depends_on(task_->rts_);
    }

    void use_device(dep::device& dev) override {
        tgt_ = &dev.get_memory_domain();
        rts_->set_device(*dev.get());
        rts_->use_device();
    }

    void use_host() override {
        tgt_ = &dep_.get_host_memory_domain();
        rts_->use_host();
    }

    void set_host_fn(std::function<void()> const& f) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        DEBUG_FMT("task[{}] uses host function {}", format::ptr(rts_.get()), format::ptr(&f));
        rts_->set_host(f);
    }

    void set_param(void const* ptr, size_t size) override {
        rts_->set_param(ptr, size);
    }

    void begin_params() override {
        g_lock.lock();
    }

    rts::memory_domain const& depends(dep::buffer& buf, dep::memory_access acc) {
        auto& buf_ = dynamic_cast<buffer_impl&>(buf);
        auto const& owner = buf_.owner();

        assert(tgt_ != nullptr);
        DEBUG_FMT("depends: task[{}] buffer[{}] target={} acc={}", format::ptr(rts_.get()),
                  format::ptr(&buf_.to_rts()), tgt_->id(), static_cast<unsigned>(acc));

        if (buf_.has_latest(tgt_->id())) {
            switch (acc) {
                case dep::memory_access::none:
                    break;

                case dep::memory_access::read_only:
                    dep_.local_read(rts_, buf, *tgt_);
                    break;

                case dep::memory_access::write_only:
                    dep_.local_write(rts_, buf, *tgt_);
                    break;

                case dep::memory_access::read_write:
                    dep_.local_read_write(rts_, buf, *tgt_);
                    break;
            }

            return *tgt_;
        } else {
            switch (acc) {
                case dep::memory_access::none:
                    break;

                case dep::memory_access::read_only:
                    dep_.transfer_read(rts_, buf, owner, *tgt_);
                    break;

                case dep::memory_access::write_only:
                    dep_.transfer_write(rts_, buf, owner, *tgt_);
                    break;

                case dep::memory_access::read_write:
                    dep_.transfer_read_write(rts_, buf, owner, *tgt_);
                    break;
            }

            return owner;
        }
    }

    void set_buffer_param(dep::buffer& buf, dep::memory_access acc, dep::id const& offset,
                          size_t offset_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));

        auto& buf_ = dynamic_cast<buffer_impl&>(buf);
        auto const& dom = depends(buf, acc);
        rts_->set_buffer_param(buf_.to_rts(), buf_.h_ptr(), dom, acc, offset, offset_byte);
    }

    void copy_1d(dep::buffer& src, dep::memory_access src_acc, size_t src_off_byte,
                 dep::buffer& dst, dep::memory_access dst_acc, size_t dst_off_byte,
                 size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& dst_ = static_cast<buffer_impl&>(dst);
        auto const& dst_dom = depends(dst, dst_acc);
        auto& src_ = static_cast<buffer_impl&>(src);
        auto const& src_dom = depends(src, src_acc);

        rts_->set_buffer_param(dst_.to_rts(), dst_.h_ptr(), dst_dom, dst_acc, rts::id(), 0);
        rts_->set_buffer_param(src_.to_rts(), src_.h_ptr(), src_dom, src_acc, rts::id(), 0);
        rts_->copy_1d(src_.to_rts(), src_off_byte, dst_.to_rts(), dst_off_byte, len_byte);
    }

    void copy_1d(dep::buffer& src, dep::memory_access src_acc, size_t src_off_byte, void* dst,
                 size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& src_ = static_cast<buffer_impl&>(src);
        auto const& src_dom = depends(src, src_acc);

        rts_->set_buffer_param(src_.to_rts(), src_.h_ptr(), src_dom, src_acc, rts::id(), 0);
        rts_->copy_1d(src_.to_rts(), src_off_byte, dst, len_byte);
    }

    void copy_1d(void const* src, dep::buffer& dst, dep::memory_access dst_acc,
                 size_t dst_off_byte, size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& dst_ = static_cast<buffer_impl&>(dst);
        auto const& dst_dom = depends(dst, dst_acc);

        rts_->set_buffer_param(dst_.to_rts(), dst_.h_ptr(), dst_dom, dst_acc, rts::id(), 0);
        rts_->copy_1d(src, dst_.to_rts(), dst_off_byte, len_byte);
    }

    void copy_2d(dep::buffer& src, dep::memory_access src_acc, size_t src_off_byte,
                 size_t src_stride, dep::buffer& dst, dep::memory_access dst_acc,
                 size_t dst_off_byte, size_t dst_stride, size_t loop,
                 size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& dst_ = static_cast<buffer_impl&>(dst);
        auto const& dst_dom = depends(dst, dst_acc);
        auto& src_ = static_cast<buffer_impl&>(src);
        auto const& src_dom = depends(src, src_acc);

        rts_->set_buffer_param(dst_.to_rts(), dst_.h_ptr(), dst_dom, dst_acc, rts::id(), 0);
        rts_->set_buffer_param(src_.to_rts(), src_.h_ptr(), src_dom, src_acc, rts::id(), 0);
        rts_->copy_2d(src_.to_rts(), src_off_byte, src_stride, dst_.to_rts(), dst_off_byte,
                      dst_stride, loop, len_byte);
    }

    void copy_2d(dep::buffer& src, dep::memory_access src_acc, size_t src_off_byte,
                 size_t src_stride, void* dst, size_t dst_stride, size_t loop,
                 size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& src_ = static_cast<buffer_impl&>(src);
        auto const& src_dom = depends(src, src_acc);

        rts_->set_buffer_param(src_.to_rts(), src_.h_ptr(), src_dom, src_acc, rts::id(), 0);
        rts_->copy_2d(src_.to_rts(), src_off_byte, src_stride, dst, dst_stride, loop, len_byte);
    }

    void copy_2d(void const* src, size_t src_stride, dep::buffer& dst,
                 dep::memory_access dst_acc, size_t dst_off_byte, size_t dst_stride,
                 size_t loop, size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& dst_ = static_cast<buffer_impl&>(dst);
        auto const& dst_dom = depends(dst, dst_acc);

        rts_->set_buffer_param(dst_.to_rts(), dst_.h_ptr(), dst_dom, dst_acc, rts::id(), 0);
        rts_->copy_2d(src, src_stride, dst_.to_rts(), dst_off_byte, dst_stride, loop, len_byte);
    }

    void copy_3d(dep::buffer& src, dep::memory_access src_acc, size_t src_off_byte,
                 size_t i_src_stride, size_t j_src_stride, dep::buffer& dst,
                 dep::memory_access dst_acc, size_t dst_off_byte, size_t i_dst_stride,
                 size_t j_dst_stride, size_t i_loop, size_t j_loop, size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& dst_ = static_cast<buffer_impl&>(dst);
        auto const& dst_dom = depends(dst, dst_acc);
        auto& src_ = static_cast<buffer_impl&>(src);
        auto const& src_dom = depends(src, src_acc);

        rts_->set_buffer_param(dst_.to_rts(), dst_.h_ptr(), dst_dom, dst_acc, rts::id(), 0);
        rts_->set_buffer_param(src_.to_rts(), src_.h_ptr(), src_dom, src_acc, rts::id(), 0);
        rts_->copy_3d(src_.to_rts(), src_off_byte, i_src_stride, j_src_stride, dst_.to_rts(),
                      dst_off_byte, i_dst_stride, j_dst_stride, i_loop, j_loop, len_byte);
    }

    void copy_3d(dep::buffer& src, dep::memory_access src_acc, size_t src_off_byte,
                 size_t i_src_stride, size_t j_src_stride, void* dst, size_t i_dst_stride,
                 size_t j_dst_stride, size_t i_loop, size_t j_loop, size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& src_ = static_cast<buffer_impl&>(src);
        auto const& src_dom = depends(src, src_acc);

        rts_->set_buffer_param(src_.to_rts(), src_.h_ptr(), src_dom, src_acc, rts::id(), 0);
        rts_->copy_3d(src_.to_rts(), src_off_byte, i_src_stride, j_src_stride, dst,
                      i_dst_stride, j_dst_stride, i_loop, j_loop, len_byte);
    }

    void copy_3d(void const* src, size_t i_src_stride, size_t j_src_stride, dep::buffer& dst,
                 dep::memory_access dst_acc, size_t dst_off_byte, size_t i_dst_stride,
                 size_t j_dst_stride, size_t i_loop, size_t j_loop, size_t len_byte) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& dst_ = static_cast<buffer_impl&>(dst);
        auto const& dst_dom = depends(dst, dst_acc);

        rts_->set_buffer_param(dst_.to_rts(), dst_.h_ptr(), dst_dom, dst_acc, rts::id(), 0);
        rts_->copy_3d(src, i_src_stride, j_src_stride, dst_.to_rts(), dst_off_byte,
                      i_dst_stride, j_dst_stride, i_loop, j_loop, len_byte);
    }

    void fill_zero(dep::buffer& dst, size_t byte_len) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        auto& dst_ = static_cast<buffer_impl&>(dst);
        depends(dst, dep::memory_access::write_only);
        rts_->fill(dst_.to_rts(), byte_len);
    }

    void set_kernel(char const* name, uint32_t hash) override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        DEBUG_FMT("task[{}] uses `{}`", format::ptr(rts_.get()), name);
        rts_->set_kernel(name, hash);
    }

    void set_desc(rts::func_desc const* desc) override {
        DEBUG_FMT("task[{}] uses descriptor `{}`", format::ptr(rts_.get()), desc->name);
        rts_->set_desc(desc);
    }

    void set_single() override {
        rts_->set_single();
    }

    void set_range(dep::range const& r) override {
        rts_->set_range(r);
    }

    void set_nd_range(dep::nd_range const& ndr) override {
        rts_->set_nd_range(ndr);
    }

    void set_local_mem_size(size_t byte) override {
        rts_->set_local_mem_size(byte);
    }

    void end_params() override {
        g_lock.unlock();
    }

    std::unique_ptr<dep::event> submit() override {
        DEBUG_FMT("task[{}] {} (this={})", format::ptr(rts_.get()), __func__,
                  format::ptr(this));
        std::shared_ptr<rts::task> t(std::move(rts_));
        return t->submit();
    }

private:
    dependency_manager_impl& dep_;
    std::shared_ptr<rts::task> rts_;
    dep::memory_domain const* tgt_ = nullptr;
};

std::shared_ptr<dep::task> dependency_manager_impl::new_task() {
    return std::make_shared<task_impl>(*this, ss_->new_task());
}

std::unique_ptr<dep::buffer> dependency_manager_impl::new_buffer(void* h_ptr,
                                                                 size_t element_size,
                                                                 rts::range size) {
    std::unique_ptr<void, delete_by_free> hp;

    if (!h_ptr) {
        hp.reset(calloc(element_size, size.size[0] * size.size[1] * size.size[2]));
        h_ptr = hp.get();
    }

    return std::make_unique<buffer_impl>(shared_from_this(), ss_->get_host_memory_domain(),
                                         h_ptr, std::move(hp),
                                         ss_->new_buffer(h_ptr, element_size, size));
}

std::vector<std::shared_ptr<dep::platform>> dependency_manager_impl::get_platforms() {
    std::vector<std::shared_ptr<dep::platform>> res;

    for (auto p : ss_->get_platforms()) {
        res.push_back(std::make_shared<platform_impl>(shared_from_this(), p));
    }

    return res;
}

void dependency_manager_impl::local_read(std::shared_ptr<rts::task> const& task,
                                         dep::buffer& buf, dep::memory_domain const& dom) {
    auto& buf_ = dynamic_cast<buffer_impl&>(buf);

    auto& ss = buf_.get_state(dom.id());

    DEBUG_FMT("buffer L-RO[{}]: v{}@dom{:x}", format::ptr(&buf_.to_rts()), ss.version(),
              dom.id());

    ss.prepare_read(task);
}

void dependency_manager_impl::local_write(std::shared_ptr<rts::task> const& task,
                                          dep::buffer& buf, dep::memory_domain const& dom) {
    auto& buf_ = dynamic_cast<buffer_impl&>(buf);

    auto& ss = buf_.get_state(dom.id());
    auto const new_ver = ss.version() + 1;

    DEBUG_FMT("buffer L-WO[{}]: (v{} -> v{})@dom{:x}", format::ptr(&buf_.to_rts()),
              ss.version(), new_ver, dom.id());

    ss.prepare_write(task, new_ver);
    buf_.set_version(dom, new_ver);
}

void dependency_manager_impl::local_read_write(std::shared_ptr<rts::task> const& task,
                                               dep::buffer& buf,
                                               dep::memory_domain const& dom) {
    local_write(task, buf, dom);
}

void dependency_manager_impl::transfer_read(std::shared_ptr<rts::task> const& task,
                                            dep::buffer& buf, dep::memory_domain const& src,
                                            dep::memory_domain const& dst) {
    auto& buf_ = dynamic_cast<buffer_impl&>(buf);

    auto& ss = buf_.get_state(src.id());
    auto& ds = buf_.get_state(dst.id());
    auto const new_ver = ss.version();

    DEBUG_FMT("buffer T-RO[{}]: v{}@dom{:x} --> (v{} -> v{})@dom{:x}",
              format::ptr(&buf_.to_rts()), ss.version(), src.id(), ds.version(), new_ver,
              dst.id());

    ss.prepare_read(task);
    ds.prepare_write(task, new_ver);
}

void dependency_manager_impl::transfer_write(std::shared_ptr<rts::task> const& task,
                                             dep::buffer& buf, dep::memory_domain const& src,
                                             dep::memory_domain const& dst) {
    auto& buf_ = dynamic_cast<buffer_impl&>(buf);

    auto& ss = buf_.get_state(src.id());
    auto& ds = buf_.get_state(dst.id());
    auto const new_ver = ss.version() + 1;

    DEBUG_FMT("buffer T-WO[{}]: v{}@dom{:x} --> (v{} -> v{})@dom{:x}",
              format::ptr(&buf_.to_rts()), ss.version(), src.id(), ds.version(), new_ver,
              dst.id());

    ds.prepare_write(task, new_ver);
    buf_.set_version(dst, new_ver);
}

void dependency_manager_impl::transfer_read_write(std::shared_ptr<rts::task> const& task,
                                                  dep::buffer& buf,
                                                  dep::memory_domain const& src,
                                                  dep::memory_domain const& dst) {
    auto& buf_ = dynamic_cast<buffer_impl&>(buf);

    auto& ss = buf_.get_state(src.id());
    auto& ds = buf_.get_state(dst.id());
    auto const new_ver = ss.version() + 1;

    DEBUG_FMT("buffer T-RW[{}]: v{}@dom{:x} --> (v{} -> v{})@dom{:x}",
              format::ptr(&buf_.to_rts()), ss.version(), src.id(), ds.version(), new_ver,
              dst.id());

    ss.prepare_read(task);
    ds.prepare_write(task, new_ver);
    buf_.set_version(dst, new_ver);
}

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE
namespace dep {

std::shared_ptr<dependency_manager> make_dependency_manager(
    std::shared_ptr<rts::subsystem> const& ss) {
    init_logging();
    return std::make_shared<dependency_manager_impl>(ss);
}

}  // namespace dep
CHARM_SYCL_END_NAMESPACE
