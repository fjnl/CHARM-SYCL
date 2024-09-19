#include <charm/sycl.hpp>
#include "rt.hpp"

namespace {

namespace runtime = sycl::runtime;
namespace impl = sycl::runtime::impl;
namespace dep = sycl::dep;

auto to_dep(sycl::access_mode mode) {
    switch (mode) {
        case sycl::access_mode::read:
            return dep::memory_access::read_only;

        case sycl::access_mode::discard_write:
            return dep::memory_access::write_only;

        case sycl::access_mode::write:
        default:
            return dep::memory_access::read_write;
    }
}

auto to_dep(sycl::id<3> const& x) {
    return dep::id(x[0], x[1], x[2]);
}

// auto to_dep(sycl::range<3> const& x) {
//     return dep::range(x[0], x[1], x[2]);
// }

unsigned int copy_dim(runtime::intrusive_ptr<runtime::accessor> const& acc_) {
    auto acc = static_pointer_cast<impl::accessor_impl>(acc_);
    auto buf = acc->get();
    auto size = buf->get_range();
    auto off = acc->get_offset();
    auto range = acc->get_range();

    auto const c2 = (size[2] == 1) || (off[2] == 0 && range[2] == size[2]);
    auto const c1 = (size[1] == 1) || (off[1] == 0 && range[1] == size[1]);
    auto const c0 = (size[0] == 1) || (off[0] == 0 && range[0] == size[0]);

    if (c2 && c1 && c0) {
        return 0;
    }
    if (c1 && c0) {
        return 1;
    }
    if (c0) {
        return 2;
    }
    return 3;
}

sycl::range<3> buf_size(runtime::intrusive_ptr<impl::accessor_impl> const& acc) {
    return acc->get()->get_range();
}

// sycl::range<3> buf_size(intrusive_ptr<runtime::accessor> const& acc_) {
//     return buf_size(static_pointer_cast<impl::accessor_impl>(acc_));
// }

size_t compute_offset(runtime::intrusive_ptr<runtime::accessor> const& acc_) {
    auto acc = static_pointer_cast<impl::accessor_impl>(acc_);
    return sycl::detail::linear_id<3>(buf_size(acc), acc->get_offset());
}

size_t elem_size(runtime::intrusive_ptr<runtime::accessor> const& acc_) {
    auto acc = static_pointer_cast<impl::accessor_impl>(acc_);
    return acc->get()->elem_size();
}

}  // namespace

CHARM_SYCL_BEGIN_NAMESPACE

namespace runtime::impl {

handler_impl::handler_impl(queue_impl& q)
    : q_(q), task_(impl::global_state::get_depmgr()->new_task()), lmem_(0) {
    if (q.profiling_enabled()) {
        task_->enable_profiling();
    }

    auto d = static_pointer_cast<impl::device_impl>(q_.get_device())->to_lower();

    if (d->is_host()) {
        task_->use_host();
    } else {
        task_->use_device(*d);
    }
}

void handler_impl::depends_on(event_ptr const& event) {
    task_->depends_on(*static_pointer_cast<dep::event>(event));
}

void handler_impl::single_task(char const* name, uint32_t hash) {
    auto d = static_pointer_cast<impl::device_impl>(q_.get_device())->to_lower();

    if (d->is_host()) {
        // TODO:
        fprintf(stderr, "not implemented: %s: %d\n", __FILE__, __LINE__);
        abort();
    }

    task_->set_kernel(name, hash);
    task_->set_single();
}

void handler_impl::parallel_for(sycl::range<3> const& range, char const* name, uint32_t hash) {
    auto d = static_pointer_cast<impl::device_impl>(q_.get_device())->to_lower();

    if (d->is_host()) {
        // TODO:
        fprintf(stderr, "not implemented: %s: %d\n", __FILE__, __LINE__);
        abort();
    }

    task_->set_kernel(name, hash);
    task_->set_range(impl::convert(range));
}

void handler_impl::parallel_for(sycl::nd_range<3> const& range, char const* name,
                                uint32_t hash) {
    auto d = static_pointer_cast<impl::device_impl>(q_.get_device())->to_lower();

    if (d->is_host()) {
        // TODO:
        fprintf(stderr, "not implemented: %s: %d\n", __FILE__, __LINE__);
        abort();
    }

    task_->set_kernel(name, hash);
    task_->set_nd_range(impl::convert(range));
    task_->set_local_mem_size(lmem_);
}

void handler_impl::set_desc(void const* desc) {
    auto d = static_pointer_cast<impl::device_impl>(q_.get_device())->to_lower();

    if (d->is_host()) {
        // TODO:
        fprintf(stderr, "not implemented: %s: %d\n", __FILE__, __LINE__);
        abort();
    }

    task_->set_desc(reinterpret_cast<rts::func_desc const*>(desc));
}

intrusive_ptr<runtime::event> handler_impl::finalize() {
    auto ev = impl::make_event(task_->submit());
    q_.add(ev);
    return ev;
}

static inline auto to_int(access_mode mode) {
    return static_cast<std::underlying_type_t<access_mode>>(mode);
}

static inline auto to_mode(std::underlying_type_t<access_mode> mode) {
    return static_cast<access_mode>(mode);
}

void handler_impl::reserve_binds(size_t n) {
    pairs_.resize(n, access_pair{});
}

void handler_impl::pre_bind(size_t idx, runtime::accessor_ptr const& acc) {
    auto acc_ = static_pointer_cast<accessor_impl>(acc);
    auto& pair = pairs_[idx];

    pair.idx = idx;
    pair.buff = acc_->get_buffer().get();
    pair.mode = acc_->get_access_mode();
}

void handler_impl::pre_bind(size_t idx, local_accessor_ptr const&) {
    pairs_[idx].idx = idx;
}

void handler_impl::pre_bind(size_t idx, void const*, size_t) {
    pairs_[idx].idx = idx;
}

void handler_impl::begin_binds() {
    task_->begin_params();
}

void handler_impl::end_pre_binds() {
    std::stable_sort(pairs_.begin(), pairs_.end());
}

void handler_impl::end_binds() {
    task_->end_params();
}

void handler_impl::bind(size_t idx, intrusive_ptr<runtime::accessor> const& acc) {
    auto acc_ = static_pointer_cast<accessor_impl>(acc);
    auto const elem = elem_size(acc_);

    auto head = std::lower_bound(pairs_.begin(), pairs_.end(),
                                 std::make_pair(idx, acc_->get_buffer().get()));
    auto const is_first = head == pairs_.begin() || head->buff != std::prev(head)->buff;
    auto dep_mode = dep::memory_access::none;

    if (is_first) {
        access_mode mode = to_mode(0);

        for (auto it = head; it != pairs_.end() && it->buff == head->buff; ++it) {
            mode = to_mode(to_int(mode) | to_int(it->mode));
        }

        dep_mode = to_dep(mode);
    }

    task_->set_buffer_param(*acc_->get()->to_lower(), dep_mode, to_dep(acc->get_offset()),
                            compute_offset(acc_) * elem);
}

void handler_impl::bind(size_t, void const* ptr, size_t size) {
    task_->set_param(ptr, size);
}

void handler_impl::bind(size_t, intrusive_ptr<runtime::local_accessor> const&) {}

void handler_impl::copy(intrusive_ptr<accessor> const& src,
                        intrusive_ptr<accessor> const& dest) {
    auto src_ = static_pointer_cast<accessor_impl>(src);
    auto dst_ = static_pointer_cast<accessor_impl>(dest);
    auto const dim = std::max(copy_dim(src_), copy_dim(dst_));
    auto const elem = elem_size(src_);
    auto src_off = compute_offset(src_) * elem;
    auto dst_off = compute_offset(dst_) * elem;
    auto copy_range = src_->get_range();

    switch (dim) {
        case 0:
        case 1: {
            auto copy_size = copy_range[0] * copy_range[1] * copy_range[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_1d(*src_->get()->to_lower(), to_dep(src_->get_access_mode()), src_off,
                           *dst_->get()->to_lower(), to_dep(dst_->get_access_mode()), dst_off,
                           copy_size);
            break;
        }

        case 2: {
            auto src_size = buf_size(src_);
            auto dst_size = buf_size(dst_);
            auto copy_size = copy_range[1] * copy_range[2] * elem;
            auto dst_stride = dst_size[1] * dst_size[2] * elem;
            auto src_stride = src_size[1] * src_size[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_2d(*src_->get()->to_lower(), to_dep(src_->get_access_mode()), src_off,
                           src_stride, *dst_->get()->to_lower(),
                           to_dep(dst_->get_access_mode()), dst_off, dst_stride, copy_range[0],
                           copy_size);
            break;
        }

        default: {
            auto src_size = buf_size(src_);
            auto dst_size = buf_size(dst_);
            auto copy_size = copy_range[2] * elem;
            auto i_dst_stride = dst_size[1] * dst_size[2] * elem;
            auto j_dst_stride = dst_size[2] * elem;
            auto i_src_stride = src_size[1] * src_size[2] * elem;
            auto j_src_stride = src_size[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_3d(*src_->get()->to_lower(), to_dep(src_->get_access_mode()), src_off,
                           i_src_stride, j_src_stride, *dst_->get()->to_lower(),
                           to_dep(dst_->get_access_mode()), dst_off, i_dst_stride, j_dst_stride,
                           copy_range[0], copy_range[1], copy_size);
            break;
        }
    }
}

void handler_impl::copy(intrusive_ptr<accessor> const& src, void* dst) {
    auto src_ = static_pointer_cast<accessor_impl>(src);
    auto const dim = copy_dim(src_);
    auto const elem = elem_size(src_);
    auto src_off = compute_offset(src_) * elem;
    auto copy_range = src_->get_range();

    switch (dim) {
        case 0:
        case 1: {
            auto copy_size = copy_range[0] * copy_range[1] * copy_range[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_1d(*src_->get()->to_lower(), to_dep(src_->get_access_mode()), src_off,
                           dst, copy_size);
            break;
        }

        case 2: {
            auto src_size = buf_size(src_);
            auto const& dst_size = copy_range;
            auto copy_size = copy_range[1] * copy_range[2] * elem;
            auto dst_stride = dst_size[1] * dst_size[2] * elem;
            auto src_stride = src_size[1] * src_size[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_2d(*src_->get()->to_lower(), to_dep(src_->get_access_mode()), src_off,
                           src_stride, dst, dst_stride, copy_range[0], copy_size);
            break;
        }

        default: {
            auto src_size = buf_size(src_);
            auto const& dst_size = copy_range;
            auto copy_size = copy_range[2] * elem;
            auto i_dst_stride = dst_size[1] * dst_size[2] * elem;
            auto j_dst_stride = dst_size[2] * elem;
            auto i_src_stride = src_size[1] * src_size[2] * elem;
            auto j_src_stride = src_size[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_3d(*src_->get()->to_lower(), to_dep(src_->get_access_mode()), src_off,
                           i_src_stride, j_src_stride, dst, i_dst_stride, j_dst_stride,
                           copy_range[0], copy_range[1], copy_size);
            break;
        }
    }
}

void handler_impl::copy(void const* src, intrusive_ptr<accessor> const& dst) {
    auto dst_ = static_pointer_cast<accessor_impl>(dst);
    auto const dim = copy_dim(dst_);
    auto const elem = elem_size(dst_);
    auto dst_off = compute_offset(dst_) * elem;
    auto copy_range = dst_->get_range();

    switch (dim) {
        case 0:
        case 1: {
            auto copy_size = copy_range[0] * copy_range[1] * copy_range[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_1d(src, *dst_->get()->to_lower(), to_dep(dst_->get_access_mode()),
                           dst_off, copy_size);
            break;
        }

        case 2: {
            auto const& src_size = copy_range;
            auto dst_size = buf_size(dst_);
            auto copy_size = copy_range[1] * copy_range[2] * elem;
            auto dst_stride = dst_size[1] * dst_size[2] * elem;
            auto src_stride = src_size[1] * src_size[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_2d(src, src_stride, *dst_->get()->to_lower(),
                           to_dep(dst_->get_access_mode()), dst_off, dst_stride, copy_range[0],
                           copy_size);
            break;
        }

        default: {
            auto const& src_size = copy_range;
            auto dst_size = buf_size(dst_);
            auto copy_size = copy_range[2] * elem;
            auto i_dst_stride = dst_size[1] * dst_size[2] * elem;
            auto j_dst_stride = dst_size[2] * elem;
            auto i_src_stride = src_size[1] * src_size[2] * elem;
            auto j_src_stride = src_size[2] * elem;

            std::scoped_lock lk(*this);

            task_->copy_3d(src, i_src_stride, j_src_stride, *dst_->get()->to_lower(),
                           to_dep(dst_->get_access_mode()), dst_off, i_dst_stride, j_dst_stride,
                           copy_range[0], copy_range[1], copy_size);
            break;
        }
    }
}

void handler_impl::fill_zero(accessor_ptr const& src, size_t len_byte) {
    auto src_ = static_pointer_cast<accessor_impl>(src)->get()->to_lower();
    task_->fill_zero(*src_, len_byte);
}

size_t handler_impl::alloc_smem(size_t byte, size_t align, bool is_array) {
    auto off = lmem_;

    if (is_array) {
        align = std::max<size_t>(align, 16);
    }

    if (lmem_ & (align - 1)) {
        off = (lmem_ + align) & ~(align - 1);
    }

    lmem_ = off + byte;

    return off;
}

}  // namespace runtime::impl

namespace runtime {

intrusive_ptr<handler> make_handler(intrusive_ptr<queue> const& queue) {
    return make_intrusive<impl::handler_impl>(*static_pointer_cast<impl::queue_impl>(queue));
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
