#pragma once

#include <BS_thread_pool.hpp>
#include "../logging.hpp"

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime {
namespace cuda {

template <class CUDA, class T = void>
struct on_device {
    using result_type = typename CUDA::deviceptr_t;

    static result_type alloc(size_t byte) {
        result_type ptr;
        if (auto err = CUDA::cu_mem_alloc(&ptr, byte); err) {
            char const* errmsg;
            CUDA::cu_get_error_string(err, &errmsg);
            FATAL("CUDA Error: cannot allocate working buffer on device: {}", errmsg);
        }
        return ptr;
    }

    static void free(result_type ptr) {
        if (ptr) {
            CUDA::cu_mem_free(ptr);
        }
    }

    T* operator()(result_type ptr) const {
        return reinterpret_cast<T*>(*ptr);
    }
};

template <class CUDA>
struct on_host {
    using result_type = void*;

    static result_type alloc(size_t byte) {
        result_type ptr;
        if (auto err = CUDA::cu_mem_alloc_host(&ptr, byte); err) {
            char const* errmsg;
            CUDA::cu_get_error_string(err, &errmsg);
            FATAL("CUDA Error: cannot allocate working buffer on pinned memory: {}", errmsg);
        }
        return ptr;
    }

    static void free(result_type ptr) {
        if (ptr) {
            CUDA::cu_mem_free_host(ptr);
        }
    }
};

template <class BLAS>
struct blas_handle {
    using result_type = typename BLAS::blas_t;

    static result_type alloc(size_t) {
        result_type h;
        if (auto err = BLAS::cublas_create(&h); err) {
            FATAL("CUDA Error: cannot create a cuBLAS handle: {}",
                  BLAS::cublas_get_status_string(err));
        }
        return h;
    }

    static void free(result_type h) {
        if (h) {
            BLAS::cublas_destroy(h);
        }
    }
};

template <class SOL>
struct dnsolver_handle {
    using result_type = typename SOL::dnsolver_t;

    static result_type alloc(size_t) {
        result_type h;
        if (auto err = SOL::cusolver_dn_create(&h); err) {
            FATAL("CUDA Error: cannot create a cuSolverDn handle: err={}",
                  static_cast<int>(*err));
        }
        return h;
    }

    static void free(result_type h) {
        if (h) {
            SOL::cusolver_dn_destroy(h);
        }
    }
};

template <class SOL>
struct dnsolver_params {
    using result_type = typename SOL::params_t;

    static result_type alloc(size_t) {
        result_type h;
        if (auto err = SOL::cusolver_dn_create_params(&h); err) {
            FATAL("CUDA Error: cannot create a cuSolverDn parameter: err={}",
                  static_cast<int>(*err));
        }
        return h;
    }

    static void free(result_type h) {
        if (h) {
            SOL::cusolver_dn_destroy_params(h);
        }
    }
};

template <class CUDA>
struct stream {
    using result_type = typename CUDA::stream_t;

    static result_type alloc(size_t) {
        result_type strm;
        if (auto err = CUDA::cu_stream_create(&strm, 0); err) {
            char const* errmsg;
            CUDA::cu_get_error_string(err, &errmsg);
            FATAL("CUDA Error: cannot create a cuStream: {}", errmsg);
        }
        return strm;
    }

    static void free(result_type strm) {
        if (strm) {
            CUDA::cu_stream_destroy(strm);
        }
    }
};

}  // namespace cuda

template <class Allocator, class T = typename Allocator::result_type>
struct work_buffer {
    ~work_buffer() {
        Allocator::free(ptr_);
    }

    auto get() {
        if (!ptr_) {
            ptr_ = Allocator::alloc(sizeof(T));
        }

        if constexpr (std::is_invocable_v<Allocator, typename Allocator::result_type>) {
            return Allocator()(ptr_);
        } else {
            return ptr_;
        }
    }

private:
    typename Allocator::result_type ptr_ = {};
};

template <class Allocator>
struct work_buffer<Allocator, std::byte*> {
    ~work_buffer() {
        Allocator::free(ptr_);
    }

    auto get(size_t byte) {
        if (alloced_ < byte) {
            if (byte & 0xfff) {
                byte = ((byte >> 12) + 1) << 12;
            }

            Allocator::free(ptr_);
            ptr_ = Allocator::alloc(byte);
            alloced_ = byte;
        }

        if constexpr (std::is_invocable_v<Allocator, typename Allocator::result_type>) {
            return Allocator()(ptr_);
        } else {
            return ptr_;
        }
    }

private:
    typename Allocator::result_type ptr_ = {};
    size_t alloced_ = 0;
};

template <class CUDA, class BLAS, class SOL>
struct cuda_context {
    typename CUDA::context_t ctx;
    typename CUDA::stream_t strm;
    work_buffer<cuda::on_device<CUDA, int>, int*> d_info;
    work_buffer<cuda::on_device<CUDA>, std::byte*> d_buffer;
    work_buffer<cuda::on_host<CUDA>, std::byte*> h_buffer;
    work_buffer<cuda::blas_handle<BLAS>> blas_handle;
    work_buffer<cuda::dnsolver_handle<SOL>> dn_handle;
    work_buffer<cuda::dnsolver_params<SOL>> dn_params;

    ~cuda_context() {
        if (strm) {
            CUDA::cu_stream_destroy(strm);
        }
    }
};

template <class CUDA, class BLAS, class SOL>
struct cuda_contexts {
    inline static typename CUDA::context_t cuda;
    inline static std::vector<std::unique_ptr<cuda_context<CUDA, BLAS, SOL>>> workspaces;

    static auto* get() {
        auto const tid = *BS::this_thread::get_index();
        auto& ptr = workspaces[tid];

        if (!ptr) {
            ptr.reset(new cuda_context<CUDA, BLAS, SOL>());
            ptr->ctx = cuda;
            CUDA::cu_ctx_set_current(ptr->ctx);
            ptr->strm = cuda::stream<CUDA>::alloc(0);
        }

        return ptr.get();
    }
};

}  // namespace runtime
CHARM_SYCL_END_NAMESPACE
