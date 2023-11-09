ARG BASE_IMAGE BASE_TAG
FROM ${BASE_IMAGE}:${BASE_TAG}

ARG ROCM=5.6

RUN apt-get update && \
    env DEBIAN_FRONTEND=noninteractive \
    apt-get install -y --no-install-recommends curl && \
    rm -rf /var/lib/apt/lists/*

RUN curl -o rocm.gpg.key https://repo.radeon.com/rocm/rocm.gpg.key && \
    echo "73f5d8100de6048aa38a8b84cd9a87f05177d208 rocm.gpg.key" | sha1sum -c && \
    gpg --dearmor < rocm.gpg.key > /etc/apt/keyrings/rocm.gpg && \
    rm rocm.gpg.key && \
    echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/rocm.gpg] https://repo.radeon.com/rocm/apt/${ROCM} jammy main" > /etc/apt/sources.list.d/rocm.list

ARG LLVM=15

RUN apt-get update && \
    env DEBIAN_FRONTEND=noninteractive \
    apt-get install -y --no-install-recommends llvm-${LLVM}-dev clang-${LLVM} libclang-${LLVM}-dev \
    clang-format-${LLVM} libomp-${LLVM}-dev make cmake ninja-build libstdc++-11-dev git \
    rocm-hip-runtime-dev${ROCM} rocm-smi${ROCM} \
    && \
    rm -rf /var/lib/apt/lists/*

ENV CC=clang-${LLVM} CXX=clang++-${LLVM} PATH=${PATH}:/opt/rocm/bin

ARG UID=1000 RENDER_GID=109

RUN useradd -m -u ${UID} user && \
    groupadd -g ${RENDER_GID} render && \
    usermod -a -G render user
USER ${UID}
