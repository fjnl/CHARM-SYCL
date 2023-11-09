#include <fstream>
#include <iostream>
#include <sstream>
#include <cxxopts.hpp>
#include <fmt/format.h>
#include <pugixml.hpp>
#include <xcml.hpp>

namespace {

bool compare_xml(pugi::xml_document const& doc1, pugi::xml_document const& doc2) {
    std::stringstream s1, s2;
    doc1.save(s1, "");
    doc2.save(s2, "");
    return s1.str() == s2.str();
}

}  // namespace

int main(int argc, char** argv) {
    cxxopts::Options options(argv[0]);
    options.add_options()("input", "input file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("o,output", "output file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("passthrough", "passthrough",
                          cxxopts::value<bool>()->default_value("false"));
    options.add_options()("check", "enable check mode",
                          cxxopts::value<bool>()->default_value("false"));
    options.parse_positional("input");
    auto opts = options.parse(argc, argv);

    auto const& input = opts["input"].as<std::string>();
    auto const& output = opts["output"].as<std::string>();
    auto const passthrough = opts["passthrough"].as<bool>();
    auto const check = opts["check"].as<bool>();

    auto doc = xcml::read_xml(input);

    if (passthrough) {
        xcml::write_xml(output, doc);
    } else {
        auto prg = xcml::xml_to_prg(doc);
        auto doc2 = xcml::prg_to_xml(prg);

        if (check) {
            auto const ok = compare_xml(doc, doc2);
            if (!ok) {
                fmt::print(stderr, "compare failed\n");
                return 1;
            }
        } else {
            xcml::write_xml(output, doc2);
        }
    }

    return 0;
}
