#pragma once

#include <complex>
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE
namespace blas {

enum class trans : unsigned int { N, T, C };

enum class uplo : unsigned int { U, L };

enum class diag : unsigned int { N, U };

enum class side : unsigned int { L, R };

}  // namespace blas
CHARM_SYCL_END_NAMESPACE

#include <charm/sycl/runtime/blas1.hpp>
#include <charm/sycl/runtime/blas2.hpp>
#include <charm/sycl/runtime/blas3.hpp>
#include <charm/sycl/runtime/lapack.hpp>
