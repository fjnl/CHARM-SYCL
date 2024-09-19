#include <utils/io.hpp>
#include "cc_kernel.hpp"
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

#ifdef CSCC_PORTABLE_MODE
extern "C" char libsycl_a_start[];
extern "C" uint64_t libsycl_a_size;
extern "C" char libcxx_a_start[];
extern "C" uint64_t libcxx_a_size;
extern "C" char libcxxabi_a_start[];
extern "C" uint64_t libcxxabi_a_size;
#endif

result<void> link_exe(config const& cfg, std::vector<io::file>& inputs) {
    cfg.begin_task("Link Executable");

    std::vector<std::string> cmd({cfg.cxx, "-o", cfg.output});
    std::vector<io::file> sycl_libs, objs;
    bool has_sycl_objects = false;
    bool uses_openmp = false;

    cmd.push_back("--driver-mode=g++");

    for (auto& input : inputs) {
        auto const targets = BOOST_LEAF_CHECK(targets_from_object(input));
        auto const is_sycl_obj = !targets.empty();

        if (is_sycl_obj) {
            has_sycl_objects = true;
            input = BOOST_LEAF_CHECK(extract_from_marked_object(cfg, input));
        }

        if (input.ext() == ".a" && is_sycl_obj) {
            sycl_libs.push_back(std::move(input));
        } else {
            objs.push_back(std::move(input));
        }

        for (auto const& target : targets) {
            if (is_openmp(target)) {
                uses_openmp = true;
            }
        }
    }

    if (!cfg.fsanitize.empty()) {
        cmd.push_back("-fsanitize=" + cfg.fsanitize);
    }

#ifndef CSCC_APPLE
    cmd.push_back("-Wl,--start-group");
#endif

    if (has_sycl_objects) {
#ifndef CSCC_APPLE
        cmd.push_back("-rdynamic");
#endif

#ifndef CSCC_APPLE
        cmd.push_back("-Wl,--whole-archive");
#endif

        for (auto const& f : sycl_libs) {
            cmd.push_back(f.filename());
        }

#ifndef CSCC_APPLE
        cmd.push_back("-Wl,--no-whole-archive");
#endif
    }

    for (auto const& obj : objs) {
        cmd.push_back(obj.filename());
    }

#ifdef CSCC_PORTABLE_MODE
    auto runtime = BOOST_LEAF_CHECK(io::file::mktemp(".a"));
    BOOST_LEAF_CHECK(runtime.write(0, libsycl_a_start, libsycl_a_size));
    cmd.push_back(runtime.filename());
#else
    auto const runtime = cfg.sycl_runtime_library();
#    ifdef CHARM_SYCL_IS_SHARED_LIBRARY
    cmd.push_back(fmt::format("-Wl,-rpath={}", runtime.parent_path()));
#    endif
    cmd.push_back(runtime);
#endif

#ifndef CSCC_APPLE
    cmd.push_back("-Wl,--end-group");
#endif

    cmd.push_back("-pthread");
    cmd.push_back("-lm");
    cmd.push_back("-ldl");
    cmd.push_back("-lrt");

    for (auto const& dir : cfg.library_dirs) {
        cmd.push_back("-L" + dir);
    }
    for (auto const& lib : cfg.libraries) {
        cmd.push_back("-l" + lib);
    }

    if (uses_openmp || cfg.host_openmp) {
        switch (BOOST_LEAF_CHECK(check_cc_vendor(cfg))) {
            case cc_vendor::gcc:
                cmd.push_back("-lgomp");
                break;

            case cc_vendor::clang:
            case cc_vendor::internal_clang:
                cmd.push_back("-fopenmp=libgomp");
                break;
        }
    }

#ifdef CSCC_PORTABLE_MODE
    auto libcxx = BOOST_LEAF_CHECK(io::file::mktemp(".a"));
    BOOST_LEAF_CHECK(libcxx.write(0, libcxx_a_start, libcxx_a_size));
    cmd.push_back(libcxx.filename());

    auto libcxxabi = BOOST_LEAF_CHECK(io::file::mktemp(".a"));
    BOOST_LEAF_CHECK(libcxxabi.write(0, libcxxabi_a_start, libcxxabi_a_size));
    cmd.push_back(libcxxabi.filename());
#endif

#ifdef CSCC_USE_LINKER
    cmd.push_back(fmt::format("-fuse-ld={}", CSCC_USE_LINKER));
#endif
#ifdef CSCC_USE_LIBCXX
    cmd.push_back(fmt::format("-stdlib=libc++"));
#endif
#ifdef CSCC_USE_LIBCXX_STATIC
    cmd.push_back(fmt::format("-static-libstdc++"));
#    ifdef CSCC_LIBCXXABI_PATH
    cmd.push_back(CSCC_LIBCXXABI_PATH);
#    else
    cmd.push_back("-lc++abi");
#    endif
#endif

    io::exec_opts opt;
    opt.umask = -1;
    if (cmd.front() == "__clang__") {
        cmd.push_back(fmt::format("-fuse-ld={}", BOOST_LEAF_CHECK(io::read_proc_self_exe())));

        BOOST_LEAF_CHECK(run_self(cfg, cmd, opt));
    } else {
        BOOST_LEAF_CHECK(run_command(cfg, cmd, opt));
    }

    cfg.end_task();

    return {};
}

result<io::file> link_so(config const& cfg, std::vector<io::file> const& inputs, bool openmp) {
    cfg.begin_task("Linking Shared Object");

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(".so"));

    std::vector<std::string> cmd({cfg.cxx, "-shared", "-o", out.filename()});

    cmd.push_back("--driver-mode=g++");

#ifdef CSCC_APPLE
    cmd.push_back("-undefined");
    cmd.push_back("dynamic_lookup");
#endif

#ifdef CSCC_USE_LINKER
    cmd.push_back(fmt::format("-fuse-ld={}", CSCC_USE_LINKER));
#endif

#ifdef CSCC_USE_LIBCXX
    cmd.push_back(fmt::format("-stdlib=libc++"));
#endif

    for (auto const& file : inputs) {
        cmd.push_back(file.filename());
    }

    if (openmp) {
        switch (BOOST_LEAF_CHECK(check_cc_vendor(cfg))) {
            case cc_vendor::gcc:
                cmd.push_back("-lgomp");
                break;

            case cc_vendor::clang:
            case cc_vendor::internal_clang:
                cmd.push_back("-fopenmp=libgomp");
                break;
        }
    }

#ifdef CSCC_USE_LIBCXX_STATIC
    cmd.push_back(fmt::format("-static-libstdc++"));
#    ifdef CSCC_LIBCXXABI_PATH
    cmd.push_back(CSCC_LIBCXXABI_PATH);
#    else
    cmd.push_back("-lc++abi");
#    endif
#endif

    if (cmd.front() == "__clang__") {
        cmd.push_back(fmt::format("-fuse-ld={}", BOOST_LEAF_CHECK(io::read_proc_self_exe())));

        BOOST_LEAF_CHECK(run_self(cfg, cmd));
    } else {
        BOOST_LEAF_CHECK(run_command(cfg, cmd));
    }
    BOOST_LEAF_CHECK(out.reopen());

    cfg.end_task();

    return out;
}
