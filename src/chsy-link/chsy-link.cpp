#include <sstream>
#include <unordered_map>
#include <cxxopts.hpp>
#include <fmt/format.h>
#include <pugixml.hpp>
#include <xcml.hpp>

namespace {

bool verbose = false;

template <class T>
struct symbol {
    std::string filename;
    std::shared_ptr<T> ptr;
};

static std::unordered_map<std::string, symbol<xcml::type_node>> defined_types;
static std::unordered_map<std::string, symbol<xcml::symbol_id>> defined_symbols;
static std::unordered_map<std::string, symbol<xcml::decl_node>> defined_definitions;
static std::unordered_map<std::string, symbol<xcml::decl_node>> defined_declarations;

template <class T = xcml::node>
inline bool compare_node(std::shared_ptr<T> x, std::shared_ptr<T> y) {
    pugi::xml_document x_doc, y_doc;

    xcml::to_xml(x_doc, x);
    xcml::to_xml(y_doc, y);

    std::stringstream xs, ys;

    x_doc.save(xs, "", pugi::format_raw);
    assert(xs.str().size() > strlen("<><>") + strlen("<?xml version=\"1.0\"?>"));

    y_doc.save(ys, "", pugi::format_raw);
    assert(ys.str().size() > strlen("<><>") + strlen("<?xml version=\"1.0\"?>"));

    return xs.str() == ys.str();
}

inline bool is_same_type(xcml::type_ptr x, xcml::type_ptr y) {
    return compare_node(x, y);
}

template <class T>
inline void add_symbol(std::unordered_map<std::string, symbol<T>>& map, std::string_view name,
                       std::shared_ptr<T> ptr, std::string_view filename) {
    symbol<T> sym;
    sym.filename = filename;
    sym.ptr = ptr;

    map.insert(std::make_pair(std::string(name), sym));
}

inline bool is_definition(xcml::decl_ptr decl) {
    auto const name = decl->node_name();
    return name == "function_definition" || name == "var_decl";
}

inline bool is_declaration(xcml::decl_ptr decl) {
    return !is_definition(decl);
}

using str_set = std::unordered_set<std::string>;

void link(xcml::xcml_program_node_ptr out_prg, xcml::xcml_program_node_ptr in_prg,
          std::string_view in_filename) {
    out_prg->gensym_id = std::max(out_prg->gensym_id, in_prg->gensym_id);

    if (!in_prg->extra.empty()) {
        if (!out_prg->extra.empty()) {
            auto const errmsg = "Error: duplicate <extra>";
            throw std::runtime_error(errmsg);
        }

        out_prg->extra = in_prg->extra;
    }

    for (auto type : in_prg->type_table) {
        auto defined = defined_types.find(type->type);

        if (defined == defined_types.end()) {
            if (verbose) {
                fmt::print("new type {} in {}\n", type->type, in_filename);
            }

            out_prg->type_table.push_back(type);
            add_symbol(defined_types, type->type, type, in_filename);
        } else if (!is_same_type(type, defined->second.ptr)) {
            auto const errmsg =
                fmt::format("Error: duplicate type {} in {}, previously defined in {}",
                            type->type, in_filename, defined->second.filename);
            throw std::runtime_error(errmsg);
        } else {
            if (verbose) {
                fmt::print("same type {} \n", type->type);
            }
        }
    }

    for (auto sym : in_prg->global_symbols) {
        auto const& name = sym->name;
        auto const defined = defined_symbols.find(name);

        if (defined == defined_symbols.end()) {
            if (verbose) {
                fmt::print("new symbol {} in {}\n", name, in_filename);
            }

            out_prg->global_symbols.push_back(sym);
            add_symbol(defined_symbols, name, sym, in_filename);
        } else if (!compare_node(sym, defined->second.ptr)) {
            auto const errmsg =
                fmt::format("Error: duplicate symbol {} in {}, previously defined in {}", name,
                            in_filename, defined->second.filename);
            throw std::runtime_error(errmsg);
        } else {
            if (verbose) {
                fmt::print("same symbol {}\n", name);
            }
        }
    }

    for (auto decl : in_prg->global_declarations) {
        if (!is_declaration(decl)) {
            continue;
        }

        auto const name = decl->name;
        auto const defined = defined_declarations.find(name);

        if (defined == defined_declarations.end()) {
            if (verbose) {
                fmt::print("new declaration {}\n", name);
            }

            out_prg->global_declarations.push_back(decl);
            add_symbol(defined_declarations, name, decl, in_filename);
        }
    }

    for (auto decl : in_prg->global_declarations) {
        if (!is_definition(decl)) {
            continue;
        }

        auto const name = decl->name;
        auto const defined = defined_definitions.find(name);

        if (defined == defined_definitions.end()) {
            if (verbose) {
                fmt::print("new declaration {}\n", name);
            }

            out_prg->global_declarations.push_back(decl);
            add_symbol(defined_definitions, name, decl, in_filename);
        } else if (!compare_node(decl, defined->second.ptr)) {
            auto const errmsg =
                fmt::format("Error: duplicate definition {} in {}, previously defined in {}",
                            name, in_filename, defined->second.filename);
            throw std::runtime_error(errmsg);
        } else {
            if (verbose) {
                fmt::print("same definition {}\n", name);
            }
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    cxxopts::Options options(argv[0]);
    options.add_options()("input", "input file",
                          cxxopts::value<std::vector<std::string>>()->default_value("-"));
    options.add_options()("o,output", "output file",
                          cxxopts::value<std::string>()->default_value("-"));
    options.add_options()("v,verbose", "enable verbose output mode",
                          cxxopts::value<bool>()->default_value("false"));
    options.parse_positional("input");
    auto opts = options.parse(argc, argv);

    auto const& input = opts["input"].as<std::vector<std::string>>();
    auto const& output = opts["output"].as<std::string>();
    verbose = opts["verbose"].as<bool>();
    auto out_prg = xcml::new_xcml_program_node();

    for (auto const& file : input) {
        if (verbose) {
            fmt::print("reading {} ...\n", file);
        }
        auto in_prg = xcml::xml_to_prg(xcml::read_xml(file));
        link(out_prg, in_prg, file);
    }

    auto out_doc = xcml::prg_to_xml(out_prg);
    xcml::write_xml(output, out_doc);

    return 0;
}
