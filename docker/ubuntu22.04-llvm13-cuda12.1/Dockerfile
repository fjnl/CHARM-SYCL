FROM nvcr.io/nvidia/nvhpc:23.5-devel-cuda12.1-ubuntu22.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
    cmake ninja-build python3 gcc g++ \
    llvm-13-dev clang-13 libclang-13-dev clang-format-13 libomp-13-dev \
    && rm -rf /var/lib/apt/lists/*

ADD . /src

ARG CMAKE_BUILD_TYPE=Release

RUN export CC=clang-13 CXX=clang++-13 PATH=${PATH}:/opt/rocm/bin && \
    cmake -S /src -B /build -G Ninja \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DBUILD_SHARED_LIBS=NO \
    -DCHARM_SYCL_CUDA_IS_REQUIRED=YES -DCHARM_SYCL_ENABLE_WERROR=YES && \
    cmake --build /build && cmake --install /build && \
    rm -rf /src /build

# smoke test
RUN cscc --version
