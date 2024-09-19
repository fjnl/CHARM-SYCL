#!/bin/bash

cd "${CI_PROJECT_DIR?}"

pwd
ls -l

echo "CHARM_SYCL_CUDA_IS_REQUIRED=${CHARM_SYCL_CUDA_IS_REQUIRED}"
echo "nvidia-smi=$(which nvidia-smi)"
echo "CHARM_SYCL_HIP_IS_REQUIRED=${CHARM_SYCL_HIP_IS_REQUIRED}"
echo "rocm-smi=$(which rocm-smi)"
echo "USE_IRIS=${USE_IRIS}"
echo "IRIS_DIR=${IRIS_DIR}"

echo
if [[ "$CHARM_SYCL_CUDA_IS_REQUIRED" = YES ]]; then
    nvidia-smi || exit $?
else
    nvidia-smi
fi

echo

if [[ "$CHARM_SYCL_HIP_IS_REQUIRED" = YES ]]; then
    rocm-smi || exit $?
else
    rocm-smi
fi
echo

rocm="$(which rocm-smi)"
if [[ -n "$rocm" ]] ;then
    rocm=$(dirname "$rocm")
    rocm=$(dirname "$rocm")
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${rocm}/lib"
fi

if [[ -n "$IRIS_DIR" ]]; then
    ls -l "$IRIS_DIR"

    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${IRIS_DIR?}/lib64:${IRIS_DIR?}/lib"
    if [[ "$CSCC_PORTABLE_MODE" = NO ]]; then
        iris_opts="-DUSE_IRIS=YES -DCHARM_SYCL_IRIS_IS_REQUIRED=YES -DIRIS_DIR=${IRIS_DIR}"
    fi
else
    iris_opts=""
fi

build_dir="${CI_PROJECT_DIR?}/build/"

export CSCC_VERBOSE=1

echo "PATH=${PATH?}"
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH?}"
echo "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE?}"
echo "CSCC_PORTABLE_MODE=${CSCC_PORTABLE_MODE?}"

cmake -S "${CI_PROJECT_DIR?}" -B "${build_dir}" -G Ninja \
    -DCHARM_SYCL_ENABLE_WERROR=ON \
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE?}" -DBUILD_SHARED_LIBS=NO \
    ${iris_opts} \
    -DCSCC_PORTABLE_MODE=${CSCC_PORTABLE_MODE?} \
    -DCHARM_SYCL_CUDA_IS_REQUIRED=${CHARM_SYCL_CUDA_IS_REQUIRED?} \
    -DCHARM_SYCL_HIP_IS_REQUIRED=${CHARM_SYCL_HIP_IS_REQUIRED?} && \
cmake --build "${build_dir}" && \
cmake --build "${build_dir}" --target check
