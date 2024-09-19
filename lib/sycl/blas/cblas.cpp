#include <blas/cblas_interface_32.hpp>
#include <blas/lapacke_interface_32.hpp>
#include "common.hpp"

CHARM_SYCL_BEGIN_NAMESPACE
namespace blas {

template <class BLAS>
result<std::string> init_cblas(void* h) {
    if (auto result = init_cblas1<BLAS>(h); !result) {
        clear_cblas1<BLAS>();
        return result;
    }

    if (auto result = init_cblas2<BLAS>(h); !result) {
        clear_cblas2<BLAS>();
        clear_cblas1<BLAS>();
        return result;
    }

    if (auto result = init_cblas3<BLAS>(h); !result) {
        clear_cblas3<BLAS>();
        clear_cblas2<BLAS>();
        clear_cblas1<BLAS>();
        return result;
    } else {
        return result;
    }
}

template result<std::string> init_cblas<runtime::cblas_interface_32>(void*);

}  // namespace blas

namespace runtime {

std::string cblas_interface_32::version() {
    return "";
}

std::string lapacke_interface_32::version() {
    return "";
}

}  // namespace runtime

CHARM_SYCL_END_NAMESPACE
