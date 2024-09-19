#pragma once

#include <array>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <charm/sycl/config.hpp>
#include "../rts.hpp"
#include "task.hpp"

CHARM_SYCL_BEGIN_NAMESPACE
namespace dev_rts {

struct coarse_task : std::enable_shared_from_this<coarse_task>, rts::task {
    coarse_task();

    ~coarse_task() = default;

    void enable_profiling() override;

    void depends_on(coarse_task const& task);

    void depends_on(rts::event const& ev) override;

    void depends_on(std::shared_ptr<task> const& task) override;

    void use_device() override;

    void set_device(rts::device& dev) override;

    void use_host() override;

    void set_kernel(char const* name, uint32_t hash) override;

    void set_host(std::function<void()> const& f) override;

    void copy_1d(rts::buffer& src, size_t src_off_byte, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override;

    void copy_1d(rts::buffer& src, size_t src_off_byte, void* dst, size_t len_byte) override;

    void copy_1d(void const* src, rts::buffer& dst, size_t dst_off_byte,
                 size_t len_byte) override;

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, rts::buffer& dst,
                 size_t dst_off_byte, size_t dst_stride, size_t loop, size_t len_byte) override;

    void copy_2d(rts::buffer& src, size_t src_off_byte, size_t src_stride, void* dst,
                 size_t dst_stride, size_t loop, size_t len_byte) override;

    void copy_2d(void const* src, size_t src_stride, rts::buffer& dst, size_t dst_off_byte,
                 size_t dst_stride, size_t loop, size_t len_byte) override;

    void copy_3d(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                 size_t j_src_stride, rts::buffer& dst, size_t dst_off_byte,
                 size_t i_dst_stride, size_t j_dst_stride, size_t i_loop, size_t j_loop,
                 size_t len_byte) override;

    void copy_3d(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                 size_t j_src_stride, void* dst, size_t i_dst_stride, size_t j_dst_stride,
                 size_t i_loop, size_t j_loop, size_t len_byte) override;

    void copy_3d(void const* src, size_t i_src_stride, size_t j_src_stride, rts::buffer& dst,
                 size_t dst_off_byte, size_t i_dst_stride, size_t j_dst_stride, size_t i_loop,
                 size_t j_loop, size_t len_byte) override;

    void fill(rts::buffer& dst, size_t byte_len) override;

    void set_desc(rts::func_desc const* desc) override;

    void set_single() override;

    void set_range(rts::range const& range) override;

    void set_nd_range(rts::nd_range const& ndr) override;

    void set_local_mem_size(size_t byte) override;

    void set_param(void const* ptr, size_t size) override;

    void commit();

    void commit(op_ptr&& op);

    void commit_as_core(op_ptr&& op);

    std::unique_ptr<rts::event> submit() override;

    struct kernel_desc {
        alignas(std::max_align_t) std::byte params[4096];
        char const* name = nullptr;
        rts::func_desc const* desc = nullptr;
        std::array<size_t, 6> range = {{0, 0, 0, 0, 0, 0}};
        size_t lmem = 0;
        std::array<void*, 64> args;
        rts::device* device = nullptr;
        uint32_t hash;
        bool is_single = false;
        bool is_ndr = false;
        bool is_device = true;
        unsigned char n_args = 0;
        std::function<void()> host_fn;
        std::byte* next_param = params;

        template <class T>
        void add_param(T const& val) {
            add_param(std::addressof(val), sizeof(T), alignof(T));
        }

        void add_param(void const* ptr, size_t byte);

        void add_param(void const* ptr, size_t byte, size_t align);

        void add_param_direct(void* ptr);
    };

protected:
    task_ptr lock_kernel_task() const;

    std::shared_ptr<kernel_desc> kdesc() {
        return std::shared_ptr<kernel_desc>(shared_from_this(), &k_);
    }

    virtual op_ptr make_kernel_op() = 0;

    virtual op_ptr make_copy_1d_op(rts::buffer& src, size_t src_off_byte, rts::buffer& dst,
                                   size_t dst_off_byte, size_t len_byte) = 0;

    virtual op_ptr make_copy_1d_op(rts::buffer& src, size_t src_off_byte, void* dst,
                                   size_t len_byte) = 0;

    virtual op_ptr make_copy_1d_op(void const* src, rts::buffer& dst, size_t dst_off_byte,
                                   size_t len_byte) = 0;

    virtual op_ptr make_copy_2d_op(rts::buffer& src, size_t src_off_byte, size_t src_stride,
                                   rts::buffer& dst, size_t dst_off_byte, size_t dst_stride,
                                   size_t loop, size_t len_byte) = 0;

    virtual op_ptr make_copy_2d_op(rts::buffer& src, size_t src_off_byte, size_t src_stride,
                                   void* dst, size_t dst_stride, size_t loop,
                                   size_t len_byte) = 0;

    virtual op_ptr make_copy_2d_op(void const* src, size_t src_stride, rts::buffer& dst,
                                   size_t dst_off_byte, size_t dst_stride, size_t loop,
                                   size_t len_byte) = 0;

    virtual op_ptr make_copy_3d_op(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                                   size_t j_src_stride, rts::buffer& dst, size_t dst_off_byte,
                                   size_t i_dst_stride, size_t j_dst_stride, size_t i_loop,
                                   size_t j_loop, size_t len_byte) = 0;

    virtual op_ptr make_copy_3d_op(rts::buffer& src, size_t src_off_byte, size_t i_src_stride,
                                   size_t j_src_stride, void* dst, size_t i_dst_stride,
                                   size_t j_dst_stride, size_t i_loop, size_t j_loop,
                                   size_t len_byte) = 0;

    virtual op_ptr make_copy_3d_op(void const* src, size_t i_src_stride, size_t j_src_stride,
                                   rts::buffer& dst, size_t dst_off_byte, size_t i_dst_stride,
                                   size_t j_dst_stride, size_t i_loop, size_t j_loop,
                                   size_t len_byte) = 0;

    virtual op_ptr make_fill_op(rts::buffer& dst, size_t byte_len) = 0;

    kernel_desc k_;

private:
    task_ptr kernel_;
    task_weak_ptr kernel_weak_;
    std::vector<task_ptr> depends_;
};

}  // namespace dev_rts
CHARM_SYCL_END_NAMESPACE
