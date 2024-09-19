#include "parse_args.hpp"
#include <utils/io.hpp>
#include <utils/target.hpp>
#include "config.hpp"

namespace {

std::string_view sv(char const* cstr) {
    return cstr;
}

bool has_next_arg(int i, int argc) {
    return i + 1 < argc;
}

[[nodiscard]] boost::leaf::result<char*> require_next_arg(int& i, int argc, char** argv) {
    if (!has_next_arg(i, argc)) {
        return BOOST_LEAF_NEW_ERROR(
            option_error("Error: option requires a value: {}", argv[i]));
    }
    i++;
    return argv[i];
}

}  // namespace

#define SHORT_OPT(name)              \
    if (!matched && arg == "-" name) \
        if ((matched = true))

#define LONG_OPT(name)                                                    \
    if (!matched && (arg.starts_with("--" name "=") || arg == "--" name)) \
        if ((matched = true))

#define SHORT_OPT_VAL(name)                                            \
    if (!matched && arg == "-" name)                                   \
        if ([[maybe_unused]] auto const optval =                       \
                sv(BOOST_LEAF_CHECK(require_next_arg(i, argc, argv))); \
            (matched = true))

#define PREFIX_OPT_VAL(name)                                                              \
    if (!matched && arg.starts_with("-" name))                                            \
        if ([[maybe_unused]] auto const optval =                                          \
                (arg == "-" name) ? sv(BOOST_LEAF_CHECK(require_next_arg(i, argc, argv))) \
                : arg.starts_with("-" name "=") ? arg.substr(strlen("-" name "="))        \
                                                : arg.substr(strlen("-" name));           \
            (matched = true))

#define LONG_OPT_VAL(name)                                                       \
    if (!matched && (arg.starts_with("-" name "=") || arg == "-" name ||         \
                     arg.starts_with("--" name "=") || arg == "--" name))        \
        if ([[maybe_unused]] auto const optval =                                 \
                arg.starts_with("-" name "=") ? arg.substr(strlen("-" name "=")) \
                : arg.starts_with("--" name "=")                                 \
                    ? arg.substr(strlen("--" name "="))                          \
                    : sv(BOOST_LEAF_CHECK(require_next_arg(i, argc, argv)));     \
            (matched = true))

boost::leaf::result<void> parse_args(int argc, char** argv, config& cfg) {
    for (int i = 1; i < argc; i++) {
        auto const arg = sv(argv[i]);
        bool matched = false;

        if (arg == "--") {
            cfg.remainder.insert(cfg.remainder.end(), argv + i + 1, argv + argc);
            break;
        }

        SHORT_OPT("c") {
            cfg.make_executable = false;
        }
        LONG_OPT_VAL("kcc") {
            cfg.k_cc = optval;
        }
        LONG_OPT_VAL("clang-format") {
            cfg.clang_format_ = optval;
        }
        SHORT_OPT_VAL("o") {
            cfg.output = optval;
        }
        SHORT_OPT("O0") {
            cfg.opt_level = '0';
        }
        SHORT_OPT("O1") {
            cfg.opt_level = '1';
        }
        SHORT_OPT("O2") {
            cfg.opt_level = '2';
        }
        SHORT_OPT("O3") {
            cfg.opt_level = '3';
        }
        SHORT_OPT("Os") {
            cfg.opt_level = 's';
        }
        SHORT_OPT("g") {
            cfg.debug = true;
        }
        SHORT_OPT("G") {
            cfg.device_debug = true;
        }
        LONG_OPT_VAL("targets") {
            if (::strcasecmp(std::string(optval).c_str(), "all") == 0) {
                cfg.targets = utils::all_targets();
            } else {
                cfg.targets = utils::from_list_string(optval);
            }

            if (std::find(cfg.targets.begin(), cfg.targets.end(), utils::target::NONE) !=
                cfg.targets.end()) {
                return BOOST_LEAF_NEW_ERROR(option_error("invalid --target value: {}", optval));
            }
        }
        PREFIX_OPT_VAL("I") {
            cfg.include_dirs.emplace_back(optval);
        }
        PREFIX_OPT_VAL("L") {
            cfg.library_dirs.emplace_back(optval);
        }
        PREFIX_OPT_VAL("l") {
            cfg.libraries.emplace_back(optval);
        }
        SHORT_OPT("v") {
            cfg.verbose = true;
        }
        LONG_OPT("verbose") {
            cfg.verbose = true;
        }
        SHORT_OPT("q") {
            cfg.verbose = false;
        }
        LONG_OPT("quiet") {
            cfg.verbose = false;
        }
        SHORT_OPT("save-temps") {
            cfg.save_temps = true;
        }
        SHORT_OPT("save-kernels") {
            cfg.save_kernels = true;
        }
        SHORT_OPT("save-xmls") {
            cfg.save_xmls = true;
        }
        PREFIX_OPT_VAL("D") {
            if (auto const eq = optval.find('='); eq != std::string_view::npos) {
                std::string k(optval.substr(0, eq)), v(optval.substr(eq + 1));
                cfg.defines.emplace(k, v);
            } else {
                cfg.defines.emplace(std::string(optval), "");
            }
        }
        LONG_OPT_VAL("fsanitize") {
            cfg.fsanitize = optval;
        }
        SHORT_OPT("MD") {
            cfg.md = true;
        }
        SHORT_OPT("MMD") {
            cfg.mmd = true;
        }
        SHORT_OPT("MM") {
            cfg.mm = true;
        }
        SHORT_OPT("M") {
            cfg.m = true;
        }
        SHORT_OPT_VAL("MF") {
            cfg.mf = optval;
        }
        SHORT_OPT_VAL("MT") {
            cfg.mt = optval;
        }
        LONG_OPT_VAL("nvcc") {
            cfg.nvcc_ = optval;
        }
        LONG_OPT_VAL("cudacxx") {
            cfg.nvcc_ = optval;
        }
        SHORT_OPT_VAL("arch") {
            cfg.cuda_arch = optval;
        }
        LONG_OPT("version") {
            cfg.show_version = true;
        }
        SHORT_OPT("fopenmp") {
            cfg.host_openmp = true;
        }

        if (matched) {
            continue;
        }

        if (arg.starts_with("-")) {
            return BOOST_LEAF_NEW_ERROR(
                option_error("{}: Error: unknown option: {}", argv[0], argv[i]));
        }

        if (auto const* val_ptr = getenv("CSCC_VERBOSE")) {
            auto val = std::string_view(val_ptr);
            if (val.size() > 0 && val != "0") {
                cfg.verbose = true;
            }
        }

        if (auto const* val_ptr = getenv("CSCC_KEEP_TEMPS")) {
            auto val = std::string_view(val_ptr);
            if (val.size() > 0 && val != "0") {
                utils::io::file::disable_removing_tempfiles();
            }
        }

        cfg.inputs.emplace_back(arg);
    }

    return cfg.validate();
}
