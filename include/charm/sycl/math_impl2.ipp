#pragma once
#include <charm/sycl.hpp>

typename sycl::vec<float, 2>::element_type __charm_sycl_length_v2f(
    typename sycl::vec<float, 2>::vector_t x0) {
    return __charm_sycl_sqrt_f(__charm_sycl_vec_ix_v2f(x0, 0) * __charm_sycl_vec_ix_v2f(x0, 0) +
                               __charm_sycl_vec_ix_v2f(x0, 1) * __charm_sycl_vec_ix_v2f(x0, 1));
}

typename sycl::vec<float, 3>::element_type __charm_sycl_length_v3f(
    typename sycl::vec<float, 3>::vector_t x0) {
    return __charm_sycl_sqrt_f(__charm_sycl_vec_ix_v3f(x0, 0) * __charm_sycl_vec_ix_v3f(x0, 0) +
                               __charm_sycl_vec_ix_v3f(x0, 1) * __charm_sycl_vec_ix_v3f(x0, 1) +
                               __charm_sycl_vec_ix_v3f(x0, 2) * __charm_sycl_vec_ix_v3f(x0, 2));
}

typename sycl::vec<float, 4>::element_type __charm_sycl_length_v4f(
    typename sycl::vec<float, 4>::vector_t x0) {
    return __charm_sycl_sqrt_f(__charm_sycl_vec_ix_v4f(x0, 0) * __charm_sycl_vec_ix_v4f(x0, 0) +
                               __charm_sycl_vec_ix_v4f(x0, 1) * __charm_sycl_vec_ix_v4f(x0, 1) +
                               __charm_sycl_vec_ix_v4f(x0, 2) * __charm_sycl_vec_ix_v4f(x0, 2) +
                               __charm_sycl_vec_ix_v4f(x0, 3) * __charm_sycl_vec_ix_v4f(x0, 3));
}

typename sycl::vec<double, 2>::element_type __charm_sycl_length_v2d(
    typename sycl::vec<double, 2>::vector_t x0) {
    return __charm_sycl_sqrt_d(__charm_sycl_vec_ix_v2d(x0, 0) * __charm_sycl_vec_ix_v2d(x0, 0) +
                               __charm_sycl_vec_ix_v2d(x0, 1) * __charm_sycl_vec_ix_v2d(x0, 1));
}

typename sycl::vec<double, 3>::element_type __charm_sycl_length_v3d(
    typename sycl::vec<double, 3>::vector_t x0) {
    return __charm_sycl_sqrt_d(__charm_sycl_vec_ix_v3d(x0, 0) * __charm_sycl_vec_ix_v3d(x0, 0) +
                               __charm_sycl_vec_ix_v3d(x0, 1) * __charm_sycl_vec_ix_v3d(x0, 1) +
                               __charm_sycl_vec_ix_v3d(x0, 2) * __charm_sycl_vec_ix_v3d(x0, 2));
}

typename sycl::vec<double, 4>::element_type __charm_sycl_length_v4d(
    typename sycl::vec<double, 4>::vector_t x0) {
    return __charm_sycl_sqrt_d(__charm_sycl_vec_ix_v4d(x0, 0) * __charm_sycl_vec_ix_v4d(x0, 0) +
                               __charm_sycl_vec_ix_v4d(x0, 1) * __charm_sycl_vec_ix_v4d(x0, 1) +
                               __charm_sycl_vec_ix_v4d(x0, 2) * __charm_sycl_vec_ix_v4d(x0, 2) +
                               __charm_sycl_vec_ix_v4d(x0, 3) * __charm_sycl_vec_ix_v4d(x0, 3));
}

typename sycl::vec<float, 2>::element_type __charm_sycl_distance_v2f(
    typename sycl::vec<float, 2>::vector_t x, typename sycl::vec<float, 2>::vector_t y) {
    return __charm_sycl_length_v2f(__charm_sycl_vec_ms_v2f(x, y));
}

typename sycl::vec<float, 3>::element_type __charm_sycl_distance_v3f(
    typename sycl::vec<float, 3>::vector_t x, typename sycl::vec<float, 3>::vector_t y) {
    return __charm_sycl_length_v3f(__charm_sycl_vec_ms_v3f(x, y));
}

typename sycl::vec<float, 4>::element_type __charm_sycl_distance_v4f(
    typename sycl::vec<float, 4>::vector_t x, typename sycl::vec<float, 4>::vector_t y) {
    return __charm_sycl_length_v4f(__charm_sycl_vec_ms_v4f(x, y));
}

typename sycl::vec<double, 2>::element_type __charm_sycl_distance_v2d(
    typename sycl::vec<double, 2>::vector_t x, typename sycl::vec<double, 2>::vector_t y) {
    return __charm_sycl_length_v2d(__charm_sycl_vec_ms_v2d(x, y));
}

typename sycl::vec<double, 3>::element_type __charm_sycl_distance_v3d(
    typename sycl::vec<double, 3>::vector_t x, typename sycl::vec<double, 3>::vector_t y) {
    return __charm_sycl_length_v3d(__charm_sycl_vec_ms_v3d(x, y));
}

typename sycl::vec<double, 4>::element_type __charm_sycl_distance_v4d(
    typename sycl::vec<double, 4>::vector_t x, typename sycl::vec<double, 4>::vector_t y) {
    return __charm_sycl_length_v4d(__charm_sycl_vec_ms_v4d(x, y));
}
