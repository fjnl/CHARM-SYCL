#!/bin/bash

cd "${CI_PROJECT_DIR?}"

pwd
ls -l

echo "CHARM_SYCL_CUDA_IS_REQUIRED=${CHARM_SYCL_CUDA_IS_REQUIRED?}"
echo "nvidia-smi=$(which nvidia-smi)"
echo "CHARM_SYCL_HIP_IS_REQUIRED=${CHARM_SYCL_HIP_IS_REQUIRED?}"
echo "rocm-smi=$(which rocm-smi)"
echo "USE_IRIS=${USE_IRIS?}"
echo "IRIS_DIR=${IRIS_DIR?}"

if [[ "$CHARM_SYCL_CUDA_IS_REQUIRED" = YES ]]; then
    if which nvidia-smi >/dev/null 2>/dev/null; then
        nvidia-smi || exit $?
        echo
    fi
fi

if [[ "$CHARM_SYCL_HIP_IS_REQUIRED" = YES ]]; then
    if which rocm-smi >/dev/null 2>/dev/null; then
        rocm-smi || exit $?
        echo
    fi
fi

build_dir="${CI_PROJECT_DIR?}/build/"

cmake -S "${CI_PROJECT_DIR?}" -B "${build_dir}" -G Ninja \
    -DCHARM_SYCL_ENABLE_WERROR=ON \
    -DCHARM_SYCL_ENABLE_ASAN=NO \
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE?}" -DBUILD_SHARED_LIBS=NO \
    -DUSE_IRIS=${USE_IRIS?} -DIRIS_DIR=${IRIS_DIR?} -DCHARM_SYCL_IRIS_IS_REQUIRED=${USE_IRIS?} \
    -DCHARM_SYCL_CUDA_IS_REQUIRED=${CHARM_SYCL_CUDA_IS_REQUIRED?} \
    -DCHARM_SYCL_HIP_IS_REQUIRED=${CHARM_SYCL_HIP_IS_REQUIRED?} && \
cmake --build "${build_dir}" && \
cmake --build "${build_dir}" --target check
