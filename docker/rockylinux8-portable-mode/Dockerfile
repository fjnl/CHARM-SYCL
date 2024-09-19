FROM rockylinux:8 as build

RUN dnf update && \
    dnf install -y 'dnf-command(config-manager)' && \
    dnf config-manager --set-enabled powertools && \
    dnf install -y cmake ninja-build perl python3 gcc gcc-c++ xz && \
    dnf clean all

RUN mkdir -p /opt/llvm/build && \
    cd /opt/llvm/build && \
    curl -sSLO https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.4/llvm-project-17.0.4.src.tar.xz && \
    echo "6703eede8013e7e5733fbd7b527757166d5698f52e239522fb320717430c88696309038501d22fe384b016b330bd92126a2c0ba88b0258c3f092801e3800b7a3  llvm-project-17.0.4.src.tar.xz" | sha512sum -c && \
    tar xf llvm-project-17.0.4.src.tar.xz

RUN cd /opt/llvm/build && \
    cd llvm-project-17.0.4.src && \
    cmake -S llvm -B build-1 -G Ninja \
    -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
    -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_WARNINGS=OFF \
    -DLLVM_ENABLE_RTTI=OFF -DLLVM_ENABLE_EH=OFF -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_INSTALL_PREFIX=/opt/llvm-1 -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON -DCLANG_DEFAULT_PIE_ON_LINUX=ON \
    && \
    ninja -C build-1 && ninja -C build-1 install && \
    rm -rf build-1

RUN cd /opt/llvm/build && \
    cd llvm-project-17.0.4.src && \
    export PATH=/opt/llvm-1/bin:/usr/local/bin:/usr/bin:/bin && \
    export CC=clang CXX=clang++ LD_LIBRARY_PATH=/opt/llvm-1/lib:/opt/llvm-1/lib/x86_64-unknown-linux-gnu:${LD_LIBRARY_PATH} && \
    cmake -S llvm -B build-2 -G Ninja \
    -DLLVM_ENABLE_PROJECTS="clang;lld" -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;compiler-rt;openmp" \
    -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_WARNINGS=OFF \
    -DLLVM_ENABLE_RTTI=ON -DLLVM_ENABLE_EH=ON -DLLVM_BUILD_LLVM_DYLIB=OFF -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_INSTALL_PREFIX=/opt/llvm -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_LINK_LLVM_DYLIB=OFF -DCLANG_LINK_CLANG_DYLIB=OFF -DOPENMP_ENABLE_LIBOMPTARGET=OFF -DLLVM_ENABLE_ZSTD=OFF \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON -DCLANG_DEFAULT_PIE_ON_LINUX=ON -DBUILD_SHARED_LIBS=OFF \
    -DLLVM_ENABLE_LIBXML2=NO -DLLVM_ENABLE_ZLIB=NO -DLLVM_ENABLE_TERMINFO=NO -DLLVM_ENABLE_LIBCXX=YES \
    && \
    ninja -C build-2 && ninja -C build-2 install && \
    cd / && \
    rm -rf /opt/llvm/build && \
    rm -rf /opt/llvm-1

ADD . /src

ARG CMAKE_BUILD_TYPE=Release

RUN export PATH=/opt/llvm/bin:${PATH} CC=clang CXX=clang++ && \
    export LD_LIBRARY_PATH=/opt/llvm/lib:/opt/llvm/lib/x86_64-unknown-linux-gnu:${LD_LIBRARY_PATH} && \
    cmake -S /src -B /build -G Ninja \
    -DBUILD_SHARED_LIBS=NO \
    -DBUILD_TESTING=NO \
    -DCHARM_SYCL_ENABLE_WERROR=YES \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
    -DCSCC_PORTABLE_MODE=YES \
    && \
    cmake --build /build && cmake --install /build

### smoke tests

FROM rockylinux:8 AS test-1
COPY --from=build /usr/local/bin/cscc /usr/local/bin
RUN cscc --version

FROM rockylinux:9 AS test-2
COPY --from=test-1 /usr/local/bin/cscc /usr/local/bin
RUN cscc --version

FROM opensuse/leap:15.5 AS test-3
COPY --from=test-2 /usr/local/bin/cscc /usr/local/bin
RUN cscc --version

FROM ubuntu:20.04 AS test-4
COPY --from=test-3 /usr/local/bin/cscc /usr/local/bin
RUN cscc --version

FROM ubuntu:22.04 AS test-5
COPY --from=test-4 /usr/local/bin/cscc /usr/local/bin
RUN cscc --version
