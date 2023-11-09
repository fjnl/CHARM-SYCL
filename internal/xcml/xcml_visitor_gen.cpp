#include <fstream>
#include <iostream>
#include <fmt/format.h>
#include "spec.hpp"

namespace {

static std::vector<char> buffer;

template <class... Args>
void pr(char const* fmt, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
        buffer.insert(buffer.end(), fmt, fmt + std::strlen(fmt));
    } else {
        fmt::format_to(std::back_inserter(buffer), fmt::runtime(fmt),
                       std::forward<Args>(args)...);
    }
}

void sep() {
    pr("\n\n");
}

void gen(std::vector<spec> const& specs) {
    pr("template <class Derived, class Return>");
    pr("struct visitor {");

    pr("template <class... Args>");
    pr("Return visit(std::shared_ptr<node> const& node, Args&&... args) {");

    std::vector<spec> filtered;

    for (auto const& spec : specs) {
        if (spec.is_binary() || spec.is_unary()) {
            continue;
        }
        filtered.push_back(spec);
    }

    for (auto const& op : {"unary_op", "binary_op"}) {
        pr("if (auto x = {}::dyncast(node)) {{", op);
        pr("return static_cast<Derived*>(this)->visit_{}(x, std::forward<Args>(args)...);", op);
        pr("}}", op);
        sep();
    }

    pr("switch (node->kind()) {");

    for (auto const& spec : filtered) {
        pr("case UINT32_C(0x{:x}):", spec.kind());
        pr("return static_cast<Derived*>(this)->visit_{}(", spec.name);
        pr("std::static_pointer_cast<{}>(node), std::forward<Args>(args)...);", spec.name);
        pr("break;", spec.kind());
        sep();
    }

    pr("default: break;");
    pr("}");
    sep();

    pr("std::string errmsg = \"unknown node: \";");
    pr("errmsg += node->node_name();");
    pr("throw std::runtime_error(errmsg);");

    pr("}");
    sep();

    for (auto const& op : {"unary_op", "binary_op"}) {
        pr("template <class... Args>");
        pr("Return visit_{}(std::shared_ptr<node>, Args&&... ) {{", op);
        pr("fprintf(stderr, \"visitor: not supported: {}\\n\");", op);
        pr("abort();");
        pr("}");
        sep();
    }

    for (auto const& spec : filtered) {
        pr("template <class... Args>");
        pr("Return visit_{}(std::shared_ptr<node>, Args&&... ) {{", spec.name);
        pr("fprintf(stderr, \"visitor: not supported: {}\\n\");", spec.name);
        pr("abort();");
        pr("}");
        sep();
    }

    pr("};");
    sep();
}

}  // namespace

int main(int argc, char** argv) {
    auto specs = load_specs();

    pr("#pragma once\n");
    sep();

    pr("namespace xcml {");
    sep();

    gen(specs);

    pr("}\n");

    if (argc == 2) {
        std::ofstream ofs(argv[1]);
        ofs.write(buffer.data(), buffer.size());
    } else {
        std::cout.write(buffer.data(), buffer.size());
    }

    return 0;
}
