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
    for (auto const& spec : specs) {
        pr("using xcml::new_{};", spec.name);
    }
    sep();

    for (auto const& spec : specs) {
        if (spec.is_unary()) {
            pr("inline {}_ptr {}(expr_ptr const& expr) {{", spec.name, spec.name);
            pr("auto ret = new_{}();", spec.name);
            pr("ret->expr = expr;");
            pr("return ret; }");
            sep();
        } else if (spec.is_binary()) {
            pr("inline {}_ptr {}(expr_ptr const& lhs, expr_ptr const& rhs) {{", spec.name,
               spec.name);
            pr("auto ret = new_{}();", spec.name);
            pr("ret->lhs = lhs; ret->rhs = rhs;");
            pr("return ret; }");
            sep();
        }
    }

    for (auto const& ty : {"char const*", "std::string const&", "std::string_view const&"}) {
        pr("inline string_constant_ptr lit({} val) {{", ty);
        pr("auto c = new_string_constant();");
        pr("c->value = val;", ty);
        pr("return c; }");
        sep();
    }

    for (std::string_view ty :
         {"int", "unsigned int", "long", "unsigned long", "long long", "unsigned long long"}) {
        pr("inline int_constant_ptr lit({} val) {{", ty);
        pr("auto c = new_int_constant();");

        auto type_name = ty.data();
        if (ty == "unsigned int") {
            type_name = "unsigned";
        } else if (ty == "unsigned long") {
            type_name = "unsigned_long";
        } else if (ty == "long long") {
            type_name = "long_long";
        } else if (ty == "unsigned long long") {
            type_name = "unsigned_long_long";
        }

        pr("c->type = \"{}\"; c->value = std::to_string(val);", type_name);
        pr("return c; }");
        sep();
    }
}

}  // namespace

int main(int argc, char** argv) {
    auto specs = load_specs();

    pr("#pragma once\n");
    sep();

    pr("#include <xcml_type.hpp>");
    sep();

    pr("namespace xcml::utils {");
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
