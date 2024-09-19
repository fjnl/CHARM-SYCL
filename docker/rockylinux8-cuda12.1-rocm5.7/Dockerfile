FROM nvcr.io/nvidia/nvhpc:23.5-devel-cuda12.1-rockylinux8

RUN echo '[amdgpu]' > /etc/yum.repos.d/amdgpu.repo && \
    echo 'name=amdgpu' >> /etc/yum.repos.d/amdgpu.repo && \
    echo 'baseurl=https://repo.radeon.com/amdgpu/5.7.3/rhel/8.8/main/x86_64' >> /etc/yum.repos.d/amdgpu.repo && \
    echo 'enabled=1' >> /etc/yum.repos.d/amdgpu.repo && \
    echo 'gpgcheck=1' >> /etc/yum.repos.d/amdgpu.repo && \
    echo 'gpgkey=https://repo.radeon.com/rocm/rocm.gpg.key' >> /etc/yum.repos.d/amdgpu.repo && \
    echo "[rocm]" > /etc/yum.repos.d/rocm.repo && \
    echo "name=rocm" >> /etc/yum.repos.d/rocm.repo && \
    echo "baseurl=https://repo.radeon.com/rocm/rhel8/5.7.3/main" >> /etc/yum.repos.d/rocm.repo && \
    echo "enabled=1" >> /etc/yum.repos.d/rocm.repo && \
    echo "priority=50" >> /etc/yum.repos.d/rocm.repo && \
    echo "gpgcheck=1" >> /etc/yum.repos.d/rocm.repo && \
    echo "gpgkey=https://repo.radeon.com/rocm/rocm.gpg.key" >> /etc/yum.repos.d/rocm.repo && \
    dnf clean all && dnf info gcc && \
    dnf install -y ninja-build python3 gcc gcc-c++ \
    llvm-devel clang-devel clang-tools-extra libomp-devel \
    rocm-hip-runtime-devel rocm-smi rocm-gdb rocprofiler roctracer rocblas-devel rocsolver-devel && \
    dnf clean all

ADD . /src

ARG CMAKE_BUILD_TYPE=Release

RUN export CC=clang CXX=clang++ PATH=${PATH}:/opt/rocm/bin && \
    cmake -S /src -B /build -G Ninja \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DBUILD_SHARED_LIBS=NO \
    -DCHARM_SYCL_CUDA_IS_REQUIRED=YES -DCHARM_SYCL_HIP_IS_REQUIRED=YES \
    -DCHARM_SYCL_ENABLE_WERROR=YES && \
    cmake --build /build && cmake --install /build && \
    rm -rf /src /build

# smoke test
RUN cscc --version
