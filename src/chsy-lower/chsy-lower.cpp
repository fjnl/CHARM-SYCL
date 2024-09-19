#include "chsy-lower.hpp"
#include <cxxopts.hpp>
#include <utils/target.hpp>
#include <xcml.hpp>

template <class F>
int run_transformation(std::string const& output, F const& fn,
                       xcml::xcml_program_node_ptr prg) {
    prg = fn(prg);
    xcml::write_xml(output, xcml::prg_to_xml(prg));
    return 0;
}

#ifdef IMPLEMENT_MAIN
int main(int argc, char** argv)
#else
int lower_main(int argc, char** argv)
#endif
{
    cxxopts::Options options(argv[0]);
    options.add_options()("input", "input file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("o,output", "output file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("t,target", "target", cxxopts::value<std::string>());
    options.parse_positional("input");
    auto opts = options.parse(argc, argv);

    auto const& input = opts["input"].as<std::string>();
    auto const& output = opts["output"].as<std::string>();
    auto const& target_str = opts["target"].as<std::string>();

    auto const target = utils::from_string(target_str);
    auto prg = xcml::xml_to_prg(xcml::read_xml(input));

    switch (target) {
        case utils::target::NONE:
            fmt::print(stderr, "{}: Unknown target: {}\n", argv[0], target_str);
            return 1;

        case utils::target::CPU_C:
            return run_transformation(output, lower_cpu_c, prg);

        case utils::target::CPU_OPENMP:
            return run_transformation(output, lower_cpu_openmp, prg);

        case utils::target::NVIDIA_CUDA:
            return run_transformation(output, lower_nvidia_cuda, prg);

        case utils::target::AMD_HIP:
            return run_transformation(output, lower_amd_hip, prg);
    }

    std::terminate();
}
