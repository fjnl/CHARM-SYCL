#include <utils/errors.hpp>
#include <charm/sycl/config.hpp>
#include "config.hpp"
#include "cscc.hpp"
#include "parse_args.hpp"
#include "task.hpp"

namespace u = utils;
namespace errs = utils::errors;
namespace io = utils::io;
using boost::leaf::result;
namespace leaf = boost::leaf;

extern "C" char iris_omp_loader_start[];
extern "C" uint64_t iris_omp_loader_size;

namespace {

// std::string_view get_ext(std::string_view filename) {
//     if (auto const pos = filename.rfind('.'); pos != std::string_view::npos) {
//         return filename.substr(pos);
//     }
//     throw std::runtime_error(std::string("No extentions found in the path:
//     ").append(filename));
// }

// bool has_ext(std::string_view filename, std::string_view ext) {
//     return get_ext(filename) == ext;
// }

struct workflow {
    explicit workflow(config const& cfg) : cfg_(cfg) {}

    [[nodiscard]] result<void> run() {
        std::vector<io::file> objects;

        for (auto const& input : cfg_.inputs) {
            auto input_file = BOOST_LEAF_CHECK(io::file::readonly(input));

            BOOST_LEAF_CHECK(process_input(objects, std::move(input_file)));
        }

        BOOST_LEAF_CHECK(link_objects(objects));

        return {};
    }

private:
    [[nodiscard]] result<void> process_input(std::vector<io::file>& objs,
                                             io::file&& input_file) {
        if (input_file.ext() == ".cpp" || input_file.ext() == ".cc") {
            BOOST_LEAF_CHECK(process_sycl_input(objs, input_file));
            return {};
        }

        if (input_file.ext() == ".o" && is_packed_file(input_file)) {
            auto files = BOOST_LEAF_CHECK(extract_packed_file(cfg_, input_file));

            for (auto& file : files) {
                objs.push_back(std::move(file));
            }

            return {};
        }

        if (input_file.ext() == ".o" || input_file.ext() == ".a") {
            objs.push_back(std::move(input_file));
            return {};
        }

        return BOOST_LEAF_NEW_ERROR(
            option_error("Not supported input file: {}", input_file.filename()));
    }

    [[nodiscard]] result<void> process_sycl_input(std::vector<io::file>& objs,
                                                  io::file const& input_file) {
        auto const dev_cpp = BOOST_LEAF_CHECK(run_cpp(cfg_, input_file, nullptr, false));

        auto const [kernel_xmls, kernel_desc] =
            BOOST_LEAF_CHECK(run_kext(cfg_, dev_cpp, cpu_symbols_));
        auto const lower_xmls = BOOST_LEAF_CHECK(run_lower(cfg_, kernel_xmls));

        auto const kernel_srcs = BOOST_LEAF_CHECK(run_cback(cfg_, lower_xmls));

        auto kernel_objs = BOOST_LEAF_CHECK(compile_kernel(kernel_srcs));

        auto host_cpp = BOOST_LEAF_CHECK(run_cpp(cfg_, input_file, &kernel_desc, true));
        auto host_obj = BOOST_LEAF_CHECK(compile_host(cfg_, host_cpp, false, false));

        objs.push_back(std::move(host_obj));
        for (auto& pair : kernel_objs) {
            objs.push_back(std::move(pair.second));
        }

        return {};
    }

    [[nodiscard]] result<io::file> embed_file(io::file const& input,
                                              std::string& prefix) const {
        auto asm_file = BOOST_LEAF_CHECK(bin2asm(cfg_, input, prefix));
        auto obj_file = BOOST_LEAF_CHECK(compile_host(cfg_, asm_file, true, false));
        return obj_file;
    }

    [[nodiscard]] result<io::file> make_binary_loader(utils::io::file const& input,
                                                      std::string_view prefix,
                                                      std::string_view kind) const {
        auto c_file = BOOST_LEAF_CHECK(make_binary_loader_source(cfg_, input, prefix, kind));
        auto obj_file = BOOST_LEAF_CHECK(compile_host(cfg_, c_file, false, false));
        return obj_file;
    }

    [[nodiscard]] result<file_map> compile_kernel(file_map const& input_files) const {
        file_map outs;

        for (auto const& [target, input] : input_files) {
            switch (target) {
                case u::target::NONE:
                    break;

                case u::target::CPU_C:
                case u::target::CPU_OPENMP: {
                    auto obj = BOOST_LEAF_CHECK(compile_kernel_cc(cfg_, input, target));
                    auto marked = BOOST_LEAF_CHECK(make_marked_object(cfg_, obj, {target}));
                    outs.emplace(target, std::move(marked));
                    continue;
                }

                case u::target::NVIDIA_CUDA: {
                    std::string prefix;

                    auto fatbin =
                        BOOST_LEAF_CHECK(compile_kernel_cuda(cfg_, input, cudafmt::FATBIN));
                    auto fatbin_obj = BOOST_LEAF_CHECK(embed_file(fatbin, prefix));
                    fatbin_obj =
                        BOOST_LEAF_CHECK(make_marked_object(cfg_, fatbin_obj, {target}));
                    auto fatbin_c =
                        BOOST_LEAF_CHECK(make_binary_loader(fatbin_obj, prefix, "_FATBIN_"));
                    outs.emplace(target, std::move(fatbin_obj));
                    outs.emplace(target, std::move(fatbin_c));

                    auto const fmt = cfg_.cuda_arch.empty() ? cudafmt::PTX : cudafmt::CUBIN;

                    auto ptx = BOOST_LEAF_CHECK(compile_kernel_cuda(cfg_, input, fmt));
                    auto ptx_obj = BOOST_LEAF_CHECK(embed_file(ptx, prefix));
                    ptx_obj = BOOST_LEAF_CHECK(make_marked_object(cfg_, ptx_obj, {target}));
                    auto ptx_c = BOOST_LEAF_CHECK(make_binary_loader(ptx_obj, prefix, "_PTX_"));
                    outs.emplace(target, std::move(ptx_obj));
                    outs.emplace(target, std::move(ptx_c));
                    continue;
                }

                case u::target::AMD_HIP: {
                    std::string prefix;

                    auto hsaco = BOOST_LEAF_CHECK(compile_kernel_hipcc(cfg_, input));
                    auto hsaco_obj = BOOST_LEAF_CHECK(embed_file(hsaco, prefix));
                    hsaco_obj = BOOST_LEAF_CHECK(make_marked_object(cfg_, hsaco_obj, {target}));
                    auto hsaco_c =
                        BOOST_LEAF_CHECK(make_binary_loader(hsaco_obj, prefix, "_HSACO_"));
                    outs.emplace(target, std::move(hsaco_obj));
                    outs.emplace(target, std::move(hsaco_c));
                    continue;
                }
            }
            std::terminate();
        }

        return outs;
    }

    [[nodiscard]] result<void> link_objects(std::vector<io::file>& objs) const {
        if (cfg_.make_executable) {
            BOOST_LEAF_CHECK(prepare_iris(objs));
        }

        if (cfg_.make_executable) {
            BOOST_LEAF_CHECK(link_exe(cfg_, objs));
        } else {
            BOOST_LEAF_CHECK(link_packed_file(cfg_, objs));
        }

        return {};
    }

    std::vector<std::string> cpu_symbols_;

    bool targets_openmp() const {
        for (auto const& t : cfg_.targets) {
            if (is_openmp(t)) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] result<void> prepare_iris(std::vector<io::file>& out_objs) const {
        cfg_.begin_task("Making IRIS OpenMP Loader");

        std::string sym_c;
        for (auto const& sym : cpu_symbols_) {
            sym_c += fmt::format("extern \"C\" void {}() {{}}\n", sym);
        }

        auto sym = BOOST_LEAF_CHECK(io::file::mktemp(".cpp"));
        cfg_.task_msg(fmt::format("Generate symbol table {}", sym.filename()));
        BOOST_LEAF_CHECK(sym.write_str(0, sym_c));
        BOOST_LEAF_CHECK(save_temps(cfg_, sym, filetype::other));

        auto omp_loader = BOOST_LEAF_CHECK(io::file::mktemp(".o"));
        BOOST_LEAF_CHECK(omp_loader.write(0, iris_omp_loader_start, iris_omp_loader_size));

        std::vector<io::file> objs;
        objs.push_back(std::move(omp_loader));
        objs.push_back(BOOST_LEAF_CHECK(compile_host(cfg_, sym, false, true)));

        auto loader = BOOST_LEAF_CHECK(link_so(cfg_, objs, targets_openmp()));
        BOOST_LEAF_CHECK(save_temps(cfg_, loader, filetype::other));
        std::string prefix;
        auto loader_obj = BOOST_LEAF_CHECK(embed_file(loader, prefix));
        auto loader_c =
            BOOST_LEAF_CHECK(make_binary_loader(loader_obj, prefix, "_IRIS_OMP_LOADER_"));
        out_objs.push_back(std::move(loader_obj));
        out_objs.push_back(std::move(loader_c));

        cfg_.end_task();

        return {};
    }

    config const& cfg_;
};

result<void> show_version([[maybe_unused]] config const& cfg) {
    printf("CHARM-SYCL Compiler %s\n", CHARM_SYCL_VERSION);

#ifdef CSCC_PORTABLE_MODE
    puts("");
    fflush(stdout);
    BOOST_LEAF_CHECK(run_self(cfg, {"__clang__", "--version"}));
    puts("");
    fflush(stdout);
    BOOST_LEAF_CHECK(run_self(cfg, {"lld", "--hash-style=gnu", "--version"}));
#endif

    return {};
}

}  // namespace

result<int> cscc_main(int argc, char** argv) {
    auto const t_start = std::chrono::high_resolution_clock::now();
    config cfg;

    BOOST_LEAF_CHECK(parse_args(argc, argv, cfg));
    if (cfg.show_version) {
        BOOST_LEAF_CHECK(show_version(cfg));
    } else {
        BOOST_LEAF_CHECK(workflow(cfg).run());
    }

    if (cfg.verbose) {
        auto const t_end = std::chrono::high_resolution_clock::now();
        fmt::print(stderr, "  TOTAL: {:.2f} sec\n", delta_sec(t_start, t_end));
    }

    return 0;
}
