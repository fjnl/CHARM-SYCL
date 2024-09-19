#pragma once

#include <boost/leaf.hpp>

boost::leaf::result<int> cscc_main(int argc, char** argv);
int lower_main(int argc, char** argv);
int kext_main(int argc, char** argv);
int get_main(int argc, char** argv);
int cback_main(int argc, char** argv);
int bin2asm_main(int argc, char** argv);

#ifdef CSCC_PORTABLE_MODE

#    if defined(__GNUC__) && !defined(__clang__)
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wunused-parameter"
#        pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#    endif
#    if defined(__GNUC__) && defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wunused-parameter"
#        pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#    endif
#    include <llvm/Support/LLVMDriver.h>

#    if defined(__GNUC__) && !defined(__clang__)
#        pragma GCC diagnostic pop
#    endif
#    if defined(__GNUC__) && defined(__clang__)
#        pragma clang diagnostic pop
#    endif

int clang_main(int argc, char** argv, llvm::ToolContext const& toolctx);
int lld_main(int argc, char** argv, llvm::ToolContext const&);
#endif
