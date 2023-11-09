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
        buffer.insert(buffer.end(), fmt, fmt + std::strlen(fmt));
    } else {
        fmt::format_to(std::back_inserter(buffer), fmt::runtime(fmt),
                       std::forward<Args>(args)...);
    }
}

void sep() {
    pr("\n\n");
}

void define_nodes() {
    pr(R"(
        struct node {
            virtual ~node() = default;
            virtual std::string_view node_name() const = 0;
            virtual std::string_view tag() const = 0;
            virtual uint32_t kind() const = 0;

            std::string file;
            int line = -1;
        };
    )");
    sep();

    pr("struct stmt_node : node {");
    pr("using node::node;");
    sep();
    pr("static auto dyncast(std::shared_ptr<node> const& obj) {");
    pr("return ((obj->kind() & UINT32_C(0x{:08x})) == UINT32_C(0x{:08x})) ? "
       "std::static_pointer_cast<stmt_node>(obj) : "
       "nullptr;",
       K_STMT, K_STMT);
    pr("}");
    sep();
    pr("};");
    sep();

    static_assert(K_STMT != 0u);

    pr("struct expr_node : stmt_node {");
    pr("using stmt_node::stmt_node;");
    sep();
    pr("static auto dyncast(std::shared_ptr<node> const& obj) {");
    pr("return ((obj->kind() & UINT32_C(0x{:08x})) == UINT32_C(0x{:08x})) ? "
       "std::static_pointer_cast<expr_node>(obj) : "
       "nullptr;",
       K_EXPR, K_EXPR);
    pr("}");
    sep();
    pr("std::string type;");
    pr("};");
    sep();

    static_assert(K_EXPR != K_STMT && K_EXPR & K_STMT);

    pr("struct {} : node {{", "decl_node");
    pr("using node::node;");
    sep();
    pr("static auto dyncast(std::shared_ptr<node> const& obj) {");
    pr("return ((obj->kind() & UINT32_C(0x{:08x})) == UINT32_C(0x{:08x})) ? "
       "std::static_pointer_cast<decl_node>(obj) : "
       "nullptr;",
       K_DECL, K_DECL);
    pr("}");
    sep();
    pr("std::string name;");
    pr("};");
    sep();

    static_assert(K_DECL != 0u);

    pr("struct {} : node {{", "type_node");
    pr("using node::node;");
    sep();
    pr("static auto dyncast(std::shared_ptr<node> const& obj) {");
    pr("return ((obj->kind() & UINT32_C(0x{:08x})) == UINT32_C(0x{:08x})) ? "
       "std::static_pointer_cast<type_node>(obj) : "
       "nullptr;",
       K_TYPE, K_TYPE);
    pr("}");
    sep();
    pr("std::string type;");
    pr("};");
    sep();

    static_assert(K_PARAM != 0u);

    pr("struct {} : node {{", "params_node");
    pr("using node::node;");
    sep();
    pr("static auto dyncast(std::shared_ptr<node> const& obj) {");
    pr("return ((obj->kind() & UINT32_C(0x{:08x})) == UINT32_C(0x{:08x})) ? "
       "std::static_pointer_cast<params_node>(obj) : "
       "nullptr;",
       K_PARAM, K_PARAM);
    pr("}");
    pr("};");
    sep();

    static_assert(K_TYPE != 0u);

    pr("struct unary_op : expr_node {");
    pr("using expr_node::expr_node;");
    sep();
    pr("std::string_view node_name() const override {");
    pr("return \"unary_op\";");
    pr("}");
    sep();
    pr("static auto dyncast(std::shared_ptr<node> const& obj) {");
    pr("return ((obj->kind() & UINT32_C(0x{:08x})) == UINT32_C(0x{:08x})) ? "
       "std::static_pointer_cast<unary_op>(obj) : "
       "nullptr;",
       K_UNARY, K_UNARY);
    pr("}");
    sep();
    pr("std::shared_ptr<expr_node> expr;");
    pr("};");
    sep();

    static_assert(K_UNARY != K_EXPR && K_UNARY & K_EXPR);

    pr("struct binary_op : expr_node {");
    pr("using expr_node::expr_node;");
    sep();
    pr("std::string_view node_name() const override {");
    pr("return \"binary_op\";");
    pr("}");
    sep();
    pr("static auto dyncast(std::shared_ptr<node> const& obj) {");
    pr("return ((obj->kind() & UINT32_C(0x{:08x})) == UINT32_C(0x{:08x})) ? "
       "std::static_pointer_cast<binary_op>(obj) : "
       "nullptr;",
       K_BINARY, K_BINARY);
    pr("}");
    sep();
    pr("std::shared_ptr<expr_node> lhs, rhs;");
    pr("};");
    sep();

    static_assert(K_BINARY != K_EXPR && K_BINARY & K_EXPR);
}

void define_nodes(std::vector<spec> const& specs) {
    for (auto const& spec : specs) {
        auto const& pt = spec.parent_type();

        pr("struct {} final : {} {{", spec.name, pt);
        pr("using {}::{};", pt, pt);
        sep();

        pr("std::string_view node_name() const override {");
        pr("return \"{}\";", spec.name);
        pr("}");
        sep();

        pr("std::string_view tag() const override {");
        pr("return \"{}\";", spec.tag);
        pr("}");
        sep();

        pr("uint32_t kind() const override {");
        pr("return UINT32_C(0x{:08x});", spec.kind());
        pr("}");
        sep();

        pr("static bool is_a(std::shared_ptr<node> const& obj){");
        pr("return obj->kind() == UINT32_C(0x{:08x});", spec.kind());
        pr("}");
        sep();

        pr("static auto dyncast(std::shared_ptr<node> const& obj){");
        pr("return is_a(obj) ? std::static_pointer_cast<{}>(obj) : nullptr;", spec.name);
        pr("}");
        sep();

        for (auto const& member : spec.members) {
            auto const& [type, name] = member;

            if (type == "string") {
                pr("std::string {};", name);
            } else if (type == "bool") {
                pr("bool {} = false;", name);
            } else if (type == "size_t") {
                pr("size_t {} = 0;", name);
            } else if (type == "size_t#") {
                pr("std::set<size_t> {};", name);
            } else if (type == "sclass") {
                pr("storage_class {} = storage_class::none;", name);
            } else if (type == "ref_scope") {
                pr("ref_scope {} = ref_scope::none;", name);
            } else if (type == "stmt") {
                pr("std::shared_ptr<stmt_node> {};", name);
            } else if (type == "expr") {
                pr("std::shared_ptr<expr_node> {};", name);
            } else if (type == "compound") {
                pr("std::shared_ptr<compound_stmt> {} = "
                   "std::make_shared<compound_stmt>();",
                   name);
            } else if (type == "symbol") {
                pr("std::shared_ptr<symbol_id> {};", name);
            } else if (type == "symbol*") {
                pr("std::vector<std::shared_ptr<symbol_id>> {};", name);
            } else if (type == "symbol%") {
                pr("std::list<std::shared_ptr<symbol_id>> {};", name);
            } else if (type == "expr*") {
                pr("std::vector<std::shared_ptr<expr_node>> {};", name);
            } else if (type == "decl*") {
                pr("std::vector<std::shared_ptr<decl_node>> {};", name);
            } else if (type == "decl%") {
                pr("std::list<std::shared_ptr<decl_node>> {};", name);
            } else if (type == "stmt%") {
                pr("std::list<std::shared_ptr<stmt_node>> {};", name);
            } else if (type == "stmt*") {
                pr("std::vector<std::shared_ptr<stmt_node>> {};", name);
            } else if (type == "type*") {
                pr("std::vector<std::shared_ptr<type_node>> {};", name);
            } else if (type == "type%") {
                pr("std::list<std::shared_ptr<type_node>> {};", name);
            } else if (type == "param*") {
                pr("std::vector<std::shared_ptr<params_node>> {};", name);
            } else if (type == "attr*") {
                pr("std::vector<std::shared_ptr<gcc_attribute>> {};", name);
            } else if (!type.empty() && *std::prev(type.end()) == '*') {
                auto type_ = std::string_view(type.c_str(), type.size() - 1);
                pr("std::vector<std::shared_ptr<{}>> {};", type_, name);
            } else {
                pr("std::shared_ptr<{}> {};", type, name);
            }
        }

        pr("};");
        sep();
    }

    for (auto const& spec : specs) {
        pr("inline std::shared_ptr<{}> new_{}() {{", spec.name, spec.name);
        pr("return std::make_shared<{}>(); }}", spec.name);
        sep();
    }
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

    pr(R"(
        #pragma once

        #include <list>
        #include <memory>
        #include <set>
        #include <string_view>
        #include <string>
        #include <vector>
        #include <xcml_type_fwd.hpp>

        namespace xcml {
    )");
    sep();

    define_nodes();
    define_nodes(specs);

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
