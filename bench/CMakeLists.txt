execute_process(
    COMMAND
    ${CMAKE_COMMAND}
    -S ${CMAKE_CURRENT_SOURCE_DIR}/vecadd
    -B ${CMAKE_CURRENT_BINARY_DIR}/vecadd
    -G ${CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_CXX_COMPILER=${LLVM_TOOLS_BINARY_DIR}/clang++
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/bench
    -DVECADD_CSCC=YES
    -DCSCC_COMMAND=${PROJECT_BINARY_DIR}/src/cscc/cscc
    COMMAND_ERROR_IS_FATAL ANY
)

add_custom_target(
    vecadd
    ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/vecadd
    USES_TERMINAL
)
add_custom_target(
    vecadd-install
    ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR}/vecadd
    DEPENDS vecadd
    USES_TERMINAL
)

add_custom_target(bench DEPENDS vecadd)

add_custom_target(bench-install DEPENDS vecadd-install)
