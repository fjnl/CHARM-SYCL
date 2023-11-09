#include <fstream>
#include <iostream>
#include <cxxopts.hpp>
#include <fmt/format.h>
#include <pugixml.hpp>

namespace {

pugi::xml_document load_xml(std::string const& input) {
    pugi::xml_document doc;
    pugi::xml_parse_result result;

    if (input == "-") {
        result = doc.load(std::cin);
    } else {
        std::ifstream fs(input);
        result = doc.load(fs);
    }

    if (!result) {
        fmt::print(stderr, "XML Error: {}\n", result.description());
        return {};
    }

    return doc;
}

int format_xml(std::string const& input) {
    auto doc = load_xml(input);

    if (doc) {
        doc.save(std::cout, "  ");
        return 0;
    }
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    cxxopts::Options options(argv[0]);
    options.add_options()("input", "input file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.parse_positional("input");
    auto opts = options.parse(argc, argv);

    auto const& input = opts["input"].as<std::string>();

    return format_xml(input);
}
