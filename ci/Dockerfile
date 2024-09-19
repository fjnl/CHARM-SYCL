FROM nvcr.io/nvidia/nvhpc:23.5-devel-cuda12.1-ubuntu22.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y curl && \
    curl -sSLO https://repo.radeon.com/rocm/rocm.gpg.key && \
    echo "73f5d8100de6048aa38a8b84cd9a87f05177d208 rocm.gpg.key" | sha1sum -c && \
    gpg --dearmor < rocm.gpg.key > /etc/apt/keyrings/rocm.gpg && \
    rm rocm.gpg.key && \
    echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/rocm.gpg] https://repo.radeon.com/rocm/apt/5.7.3 jammy main" > /etc/apt/sources.list.d/rocm.list && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
    cmake ninja-build python3 gcc g++ \
    llvm-13-dev clang-13 libclang-13-dev clang-format-13 libomp-13-dev \
    rocm-hip-runtime-dev5.7.3 rocm-smi5.7.3 rocm-gdb5.7.3 rocprofiler5.7.3 roctracer5.7.3 && \
    apt-get autoremove -y curl && \
    rm -rf /var/lib/apt/lists/*

ENV CC=clang-13 CXX=clang++-13 PATH=${PATH}:/opt/rocm/bin

RUN useradd -m -u 1000 user && \
    groupadd -g 109 render && \
    usermod -a -G render user

USER 1000
