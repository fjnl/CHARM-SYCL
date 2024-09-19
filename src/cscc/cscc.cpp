#include "cscc.hpp"
#include <cstring>
#include <string_view>
#include <boost/assert.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <utils/errors.hpp>
#include <utils/io.hpp>

namespace {

template <class F>
int try_handle_all(char const* argv0, F&& f) {
    return boost::leaf::try_handle_all(
        [&]() -> boost::leaf::result<int> {
            return utils::io::try_handle_some(argv0, [&]() -> boost::leaf::result<int> {
                return utils::errors::try_handle_some(argv0, f);
            });
        },
        [&](boost::leaf::verbose_diagnostic_info const& diag) -> int {
            utils::errors::report_and_exit(argv0, diag);
        });
}

}  // namespace

namespace boost {
void assertion_failed(char const* expr, char const* function, char const* file, long line) {
    fmt::print(stderr, fg(fmt::color::red), "Assersion failed: {} at {}:{} in function '{}'\n",
               expr, file, line, function);
    std::abort();
}
}  // namespace boost

int main(int argc, char** argv) {
    auto argv0 = std::string_view(argv[0]);

    if (argv0 == "__chsy_lower__") {
        return try_handle_all(argv[0], [&]() -> boost::leaf::result<int> {
            return lower_main(argc, argv);
        });
    }

    if (argv0 == "__chsy_kext__") {
        return try_handle_all(argv[0], [&]() -> boost::leaf::result<int> {
            return kext_main(argc, argv);
        });
    }

    if (argv0 == "__chsy_get__") {
        return try_handle_all(argv[0], [&]() -> boost::leaf::result<int> {
            return get_main(argc, argv);
        });
    }

    if (argv0 == "__chsy_cback__") {
        return try_handle_all(argv[0], [&]() -> boost::leaf::result<int> {
            return cback_main(argc, argv);
        });
    }

    if (argv0 == "__chsy_bin2asm__") {
        return try_handle_all(argv[0], [&]() -> boost::leaf::result<int> {
            return bin2asm_main(argc, argv);
        });
    }

#ifdef CSCC_PORTABLE_MODE
    llvm::ToolContext toolctx;

    toolctx.Path = argv[0];
    toolctx.PrependArg = nullptr;
    toolctx.NeedsPrependArg = false;

    if (argv0 == "__clang__" || (argc >= 1 && strcmp(argv[1], "-cc1") == 0)) {
        return clang_main(argc, argv, toolctx);
    }

    bool is_lld = false;
    for (int i = 1; i < argc; ++i) {
        auto argi = std::string_view(argv[i]);
        if (argi == "-dynamic-linker" || argi == "--dynamic-linker" ||
            argi == "-no-dynamic-linker" || argi == "--no-dynamic-linker" ||
            argi.starts_with("--hash-style")) {
            is_lld = true;
            break;
        }
    }

    if (is_lld) {
#    ifdef CSCC_APPLE
        argv[0] = strdup("ld64.lld");
#    else
        argv[0] = strdup("ld.lld");
#    endif

        return lld_main(argc, argv, toolctx);
    }
#endif

    return try_handle_all(argv[0], [&]() -> boost::leaf::result<int> {
        return cscc_main(argc, argv);
    });
}
