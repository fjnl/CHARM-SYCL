#pragma once

#include <BS_thread_pool.hpp>
#include "../cuda/context.hpp"
#include "../logging.hpp"

CHARM_SYCL_BEGIN_NAMESPACE
namespace runtime {
namespace hip {

template <class HIP, class T = void>
struct on_device {
    using result_type = typename HIP::deviceptr_t;

    static result_type alloc(size_t byte) {
        result_type ptr;
        if (auto err = HIP::hip_malloc(&ptr, byte); err) {
            FATAL("HIP Error: cannot allocate working buffer on device: {}",
                  HIP::hip_get_error_string(err));
        }
        return ptr;
    }

    static void free(result_type ptr) {
        if (ptr) {
            HIP::hip_free(ptr);
        }
    }

    T* operator()(result_type ptr) const {
        return reinterpret_cast<T*>(*ptr);
    }
};

template <class BLAS>
struct blas_handle {
    using result_type = typename BLAS::handle_t;

    static result_type alloc(size_t) {
        result_type h;
        if (auto err = BLAS::rocblas_create_handle(&h); err) {
            FATAL("HIP Error: cannot create a cuBLAS handle: {}",
                  BLAS::rocblas_status_to_string(err));
        }
        return h;
    }

    static void free(result_type h) {
        BLAS::rocblas_destroy_handle(h);
    }
};

template <class HIP>
struct stream {
    using result_type = typename HIP::stream_t;

    static result_type alloc(size_t) {
        result_type strm;
        if (auto err = HIP::hip_stream_create(&strm); err) {
            FATAL("HIP Error: cannot create a cuStream: {}", HIP::hip_get_error_string(err));
        }
        return strm;
    }

    static void free(result_type strm) {
        HIP::hip_stream_destroy(strm);
    }
};

}  // namespace hip

template <class HIP, class BLAS, class SOL>
struct hip_context {
    typename HIP::stream_t strm;
    work_buffer<hip::on_device<HIP, int>, int*> d_info;
    work_buffer<hip::blas_handle<BLAS>> blas_handle;

    ~hip_context() {
        if (strm) {
            hip::stream<HIP>::free(strm);
        }
    }
};

template <class HIP, class BLAS, class SOL>
struct hip_contexts {
    inline static std::vector<std::unique_ptr<hip_context<HIP, BLAS, SOL>>> workspaces;

    static auto* get() {
        auto const tid = *BS::this_thread::get_index();
        auto& ptr = workspaces[tid];

        if (!ptr) {
            ptr.reset(new hip_context<HIP, BLAS, SOL>());
            ptr->strm = hip::stream<HIP>::alloc(0);
        }

        return ptr.get();
    }
};

}  // namespace runtime
CHARM_SYCL_END_NAMESPACE
