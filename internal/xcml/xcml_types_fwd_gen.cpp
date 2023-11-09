#include <algorithm>
#include <fstream>
#include <iostream>
#include <fmt/format.h>
#include "spec.hpp"

namespace {

static std::vector<char> buffer;

template <class... Args>
void pr(char const* fmt, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
        fmt::format_to(std::back_inserter(buffer), "{}", fmt);
    } else {
        fmt::format_to(std::back_inserter(buffer), fmt::runtime(fmt),
                       std::forward<Args>(args)...);
    }
}

void sep() {
    pr("\n\n");
}

void fwds(std::vector<spec> const& specs) {
    pr("enum class storage_class {");
    for (auto const& v : {"none", "auto_", "param", "extern_", "extern_def", "static_",
                          "register_", "label_", "tagname", "moe", "typedef_name"}) {
        pr("{},", v);
    }
    pr("};");
    sep();

    pr("enum class ref_scope {");
    for (auto const& v : {"none", "global", "local", "param"}) {
        pr("{},", v);
    }
    pr("};");
    sep();

    pr("struct node;");
    pr("struct stmt_node;");
    pr("struct expr_node;");
    pr("struct decl_node;");
    pr("struct type_node;");

    for (auto const& spec : specs) {
        pr("struct {};\n", spec.name);
    }
    sep();
}

void define_aliases(std::vector<spec> const& specs) {
    auto const alias = [](std::string_view x, std::string_view y) {
        pr("using {}_ptr = std::shared_ptr<{}>;", x, y);
    };

    alias("node", "node");
    alias("stmt", "stmt_node");
    alias("expr", "expr_node");
    alias("decl", "decl_node");
    alias("type", "type_node");

    for (auto const& spec : specs) {
        alias(spec.name, spec.name);
    }

    sep();
}

}  // namespace

int main(int argc, char** argv) {
    auto specs = load_specs();

    std::sort(specs.begin(), specs.end(), [](auto const& x, auto const& y) {
        if (x.name == "xcml_program_node") {
            return false;
        }
        if (y.name == "xcml_program_node") {
            return true;
        }
        return x.name < y.name;
    });

    pr("#pragma once\n");
    sep();

    pr("#include <memory>");
    sep();

    pr("namespace xcml {");
    sep();

    fwds(specs);
    define_aliases(specs);

    sep();
    pr("}\n");

    if (argc == 2) {
        std::ofstream ofs(argv[1]);
        ofs.write(buffer.data(), buffer.size());
    } else {
        std::cout.write(buffer.data(), buffer.size());
    }

    return 0;
}
