#pragma once

#include <string>
#include <dlfcn.h>
#include <charm/sycl/runtime/blas.hpp>
#include "../error.hpp"
#include "../format.hpp"
#include "../logging.hpp"
#include "../rt.hpp"
#include "../rts.hpp"
#include "blas.hpp"

CHARM_SYCL_BEGIN_NAMESPACE

namespace blas {

using CHARM_SYCL_NS::error::result;
namespace error = CHARM_SYCL_NS::error;

inline result<void*> func(void* h, char const* name) {
    auto const fn = dlsym(h, name);
    if (!fn) {
        return error::make_errorf("cannot load function {}: {}", name, dlerror());
    }
    return fn;
}

inline void* try_dlopen(char const* path, std::string& errmsg) {
    void* h = dlopen(path, RTLD_GLOBAL | RTLD_NOW);
    std::string err;

    if (!h) {
        if (!errmsg.empty()) {
            errmsg += '\n';
        }
        err = dlerror();
        errmsg += err;
    }

    if (h) {
        INFO("{} is loaded", path);
    } else {
        WARN("Unable to load: {}", path, err);
    }

    return h;
}

inline unsigned int trans_to_int(trans x) {
    switch (x) {
        case trans::N:
            return 111;
        case trans::T:
            return 112;
        case trans::C:
            return 113;
    }
}

inline unsigned int uplo_to_int(uplo x) {
    switch (x) {
        case uplo::U:
            return 121;
        case uplo::L:
            return 122;
    }
}

inline unsigned int diag_to_int(diag x) {
    switch (x) {
        case diag::N:
            return 131;
        case diag::U:
            return 132;
    }
}

inline unsigned int side_to_int(side x) {
    switch (x) {
        case side::L:
            return 141;
        case side::R:
            return 142;
    }
}

inline char trans_to_char(trans x) {
    switch (x) {
        case trans::N:
            return 'N';
        case trans::T:
            return 'T';
        case trans::C:
            return 'C';
    }
}

inline char uplo_to_char(uplo x) {
    switch (x) {
        case uplo::U:
            return 'U';
        case uplo::L:
            return 'L';
    }
}

inline char diag_to_char(diag x) {
    switch (x) {
        case diag::N:
            return 'N';
        case diag::U:
            return 'U';
    }
}

inline char side_to_char(side x) {
    switch (x) {
        case side::L:
            return 'L';
        case side::R:
            return 'R';
    }
}

template <class T>
inline auto trans_into(trans x) {
    switch (x) {
        case trans::N:
            return T::TRANS_N;
        case trans::T:
            return T::TRANS_T;
        case trans::C:
            return T::TRANS_C;
    }
}

template <class T>
inline auto uplo_into(uplo x) {
    switch (x) {
        case uplo::U:
            return T::UPLO_U;
        case uplo::L:
            return T::UPLO_L;
    }
}

template <class T>
inline auto diag_into(diag x) {
    switch (x) {
        case diag::N:
            return T::DIAG_N;
        case diag::U:
            return T::DIAG_U;
    }
}

template <class T>
inline auto side_into(side x) {
    switch (x) {
        case side::L:
            return T::SIDE_L;
        case side::R:
            return T::SIDE_R;
    }
}

template <int Width, class T>
inline auto size_cast(T t, char const* fn, char const* var, char const* note) {
    using U = std::conditional_t<Width == 32, int32_t, int64_t>;
    using UU = std::make_unsigned_t<U>;
    using TL = std::numeric_limits<T>;
    using UL = std::numeric_limits<U>;

    if constexpr (TL::is_signed) {
        if constexpr (UL::min() <= TL::min() && TL::max() <= UL::max()) {
            return static_cast<U>(t);
        } else {
            if (static_cast<T>(UL::min()) <= t && t <= static_cast<T>(UL::max())) {
                return static_cast<U>(t);
            }
        }
    } else {
        if constexpr (TL::max() <= static_cast<UU>(UL::max())) {
            return static_cast<U>(t);
        } else {
            if (t <= static_cast<UU>(UL::max())) {
                return static_cast<U>(t);
            }
        }
    }

    format::print(std::cerr, "integer overflow {} while calling {} ({}).", var, fn, note);
    std::abort();
}

result<std::string> init_refblas();
result<std::string> init_openblas();
result<std::string> init_mkl();
result<std::string> init_cublas();
result<std::string> init_rocblas();

result<std::string> init_lapack_refblas();
result<std::string> init_lapack_openblas();
result<std::string> init_lapack_mkl();
result<std::string> init_lapack_cublas();
result<std::string> init_lapack_rocblas();

inline struct cblas_tag {
} cblas_tag;

template <class BLAS>
result<std::string> init_cblas(void* h);

template <class BLAS>
result<std::string> init_cblas1(void* h);

template <class BLAS>
void clear_cblas1();

template <class BLAS>
result<std::string> init_cblas2(void* h);

template <class BLAS>
void clear_cblas2();

template <class BLAS>
result<std::string> init_cblas3(void* h);

template <class BLAS>
void clear_cblas3();

inline struct lapacke_tag {
} lapacke_tag;

template <class LAPACKE>
result<std::string> init_lapacke(void* h);

template <class LAPACKE>
void clear_lapacke();

inline struct cublas_tag {
} cublas_tag;

template <class BLAS>
error::result<std::string> init_cublas1();

template <class BLAS>
void clear_cublas1();

template <class BLAS>
error::result<std::string> init_cublas2();

template <class BLAS>
void clear_cublas2();

template <class BLAS>
error::result<std::string> init_cublas3();

template <class BLAS>
void clear_cublas3();

inline struct cusolver_tag {
} cusolver_tag;

template <class SOL>
result<std::string> init_cusolver_lapack();

template <class SOL>
void clear_cusolver_lapack();

inline struct rocblas_tag {
} rocblas_tag;

template <class BLAS>
error::result<std::string> init_rocblas1();

template <class BLAS>
void clear_rocblas1();

template <class BLAS>
error::result<std::string> init_rocblas2();

template <class BLAS>
void clear_rocblas2();

template <class BLAS>
error::result<std::string> init_rocblas3();

template <class BLAS>
void clear_rocblas3();

inline struct rocsolver_tag {
} rocsolver_tag;

template <class SOL>
result<std::string> init_rocsolver_lapack();

template <class SOL>
void clear_rocsolver_lapack();

template <class F>
static error::result<void> load_func(void* handle, F& fn, char const* name) {
    fn = reinterpret_cast<F>(dlsym(handle, name));

    if (!fn) {
        return error::make_errorf("Cannot load function: {}: {}", name, dlerror());
    }

    return {};
}

}  // namespace blas

CHARM_SYCL_END_NAMESPACE
