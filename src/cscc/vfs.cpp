#ifndef CSCC_PORTABLE_MODE
#    error Invalid Configuration: CSCC_PORTABLE_MODE is not defined
#endif

#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/VirtualFileSystem.h>

// clang-format off
#include <vfs.ipp>
// clang-format on

void cscc_install_vfs(clang::CompilerInstance& clang) {
    clang.createFileManager();

    auto& fm = clang.getFileManager();
#if LLVM_VERSION_MAJOR >= 16
    auto basefs = fm.getVirtualFileSystemPtr();
#else
    auto basefs = llvm::IntrusiveRefCntPtr(&fm.getVirtualFileSystem());
#endif
    auto overlay = llvm::IntrusiveRefCntPtr(new llvm::vfs::OverlayFileSystem(basefs));
    auto in_memory = llvm::IntrusiveRefCntPtr(new llvm::vfs::InMemoryFileSystem());

    add_files(*in_memory);

    overlay->pushOverlay(in_memory);
    fm.setVirtualFileSystem(overlay);
}
