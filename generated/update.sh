#!/bin/bash

set -e

cargo test
cargo build

cp ../.clang-format .
echo "MaxEmptyLinesToKeep: 0" >> .clang-format

mkdir -p include/charm/sycl
mkdir -p lib/sycl/cuda

rm -f include/charm/sycl/id_*.hpp
rm -f include/charm/sycl/id_*.ipp
rm -f lib/sycl/cuda/cuda_interface.hpp
rm -f lib/sycl/cuda/cuda_interface_vars.ipp

for d in 1 2 3; do
    ./target/debug/id-range-gen --decl --target id    --dim ${d} --output include/charm/sycl/id_${d}.hpp
    clang-format -i include/charm/sycl/id_${d}.hpp
    wc -l include/charm/sycl/id_${d}.hpp

    ./target/debug/id-range-gen        --target id    --dim ${d} --output include/charm/sycl/id_${d}.ipp
    clang-format -i include/charm/sycl/id_${d}.ipp
    wc -l include/charm/sycl/id_${d}.ipp

    ./target/debug/id-range-gen --decl --target range --dim ${d} --output include/charm/sycl/range_${d}.hpp
    clang-format -i include/charm/sycl/range_${d}.hpp
    wc -l include/charm/sycl/range_${d}.hpp

    ./target/debug/id-range-gen        --target range --dim ${d} --output include/charm/sycl/range_${d}.ipp
    clang-format -i include/charm/sycl/range_${d}.ipp
    wc -l include/charm/sycl/range_${d}.ipp
done

./target/debug/cuda-ifgen --output lib/sycl/cuda/cuda_interface.hpp
clang-format -i lib/sycl/cuda/cuda_interface.hpp
wc -l lib/sycl/cuda/cuda_interface.hpp

./target/debug/cuda-ifgen --vars --output lib/sycl/cuda/cuda_interface_vars.ipp
clang-format -i lib/sycl/cuda/cuda_interface_vars.ipp
wc -l lib/sycl/cuda/cuda_interface_vars.ipp

if [[ -e ../.git ]]; then
    echo
    git status .
fi
