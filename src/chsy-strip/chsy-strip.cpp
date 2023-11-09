#include <cxxopts.hpp>
#include <xcml.hpp>

int main(int argc, char** argv) {
    cxxopts::Options options(argv[0]);
    options.add_options()("input", "input file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("o,output", "output file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("d,declarations", "remove global declarations",
                          cxxopts::value<bool>()->default_value("false"));
    options.add_options()("x,extra", "remove extra information",
                          cxxopts::value<bool>()->default_value("false"));
    options.parse_positional("input");
    auto opts = options.parse(argc, argv);

    auto const& input = opts["input"].as<std::string>();
    auto const& output = opts["output"].as<std::string>();

    auto doc = xcml::read_xml(input);
    if (opts["declarations"].as<bool>()) {
        doc.child("XcodeProgram").child("globalDeclarations").remove_children();
    }
    if (opts["extra"].as<bool>()) {
        doc.child("XcodeProgram").remove_child("extra");
    }
    xcml::write_xml(output, doc);

    return 0;
}
