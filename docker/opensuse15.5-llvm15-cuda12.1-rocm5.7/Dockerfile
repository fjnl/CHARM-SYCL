FROM opensuse/leap:15.5

RUN zypper addrepo 'https://developer.download.nvidia.com/compute/cuda/repos/opensuse15/x86_64/cuda-opensuse15.repo' && \
    echo '[amdgpu]' > /etc/zypp/repos.d/amdgpu.repo && \
    echo 'name=amdgpu' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'baseurl=https://repo.radeon.com/amdgpu/5.7.3/sle/15.5/main/x86_64/' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'enabled=1' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'gpgcheck=1' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'gpgkey=https://repo.radeon.com/rocm/rocm.gpg.key' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo '[ROCm-5.7.3]' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'name=ROCm5.7.3' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'baseurl=https://repo.radeon.com/rocm/zyp/5.7.3/main' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'enabled=1' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'gpgcheck=1' >> /etc/zypp/repos.d/amdgpu.repo && \
    echo 'gpgkey=https://repo.radeon.com/rocm/rocm.gpg.key' >> /etc/zypp/repos.d/amdgpu.repo && \
    zypper addrepo 'https://download.opensuse.org/repositories/devel:/languages:/perl/15.5/devel:languages:perl.repo' && \
    zypper --gpg-auto-import-keys refresh && \
    zypper install -y gcc11 gcc11-c++ cmake ninja python3 \
    llvm-devel clang-devel clang-tools \
    cuda-12-1 \
    rocm-hip-runtime-devel rocm-gdb rocprofiler roctracer rocblas-devel rocsolver-devel \
    && \
    zypper clean --all

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
