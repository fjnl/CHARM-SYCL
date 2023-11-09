#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>
#include <fmt/format.h>
#include "spec.hpp"

namespace {

std::vector<char> buffer;
std::unordered_map<std::string, std::string> g_tagmap;

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

std::unordered_map<std::string, std::string> make_tag_map(std::vector<spec> const& specs) {
    std::unordered_map<std::string, std::string> map;

    for (auto const& s : specs) {
        map.insert_or_assign(s.name, s.tag);
    }
    return map;
}

std::string_view rename(std::string_view name) {
    auto const len = name.size();
    if (!name.empty() && name.at(len - 1) == '_') {
        return name.substr(0, len - 1);
    }
    return name;
}

struct generator {
    int id_ = 0;

    void operator()(tag_node const& node) {
        id_ = 1;
        (*this)(0, node);
    }

    void operator()(int pid, tag_node const& node) {
        auto const id = id_++;

        if (pid == 0) {
            auto const& tag = g_tagmap.at(node.var);
            pr("[[maybe_unused]] auto node{} = node{}.append_child(\"{}\");", id, pid, tag);
            pr(R"(
                if (!obj->file.empty()) {
                    node1.append_attribute("file") = obj->file.c_str();
                }
                if (obj->line >= 0) {
                    node1.append_attribute("line") = obj->line;
                }
            )");
        } else {
            pr("[[maybe_unused]] auto node{} = node{}.append_child(\"{}\");", id, pid,
               rename(node.var));
        }

        for (auto const& attr : node.attrs) {
            (*this)(id, attr);
        }
        for (auto const& child : node.body) {
            (*this)(id, *child);
        }
    }

    void operator()(int pid, xml_node const& nd) {
        if (auto ifn = dynamic_cast<if_node const*>(&nd)) {
            pr("if (test(obj->{})) {{", ifn->var);
            for (auto const& child : ifn->body) {
                (*this)(pid, *child);
            }
            pr("}");
        } else if (auto fn = dynamic_cast<foreach_node const*>(&nd)) {
            pr("for (auto const& c : obj->{}) {{", fn->var);
            pr("to_XML(node{}, c);", pid);
            pr("}");
        } else if (auto rn = dynamic_cast<ref_node const*>(&nd)) {
            pr("to_XML(node{}, obj->{});", pid, rn->var);
        } else if (auto nn = dynamic_cast<nodes_node const*>(&nd)) {
            pr("for (auto const& c : obj->{}) {{", nn->var);
            pr("to_XML(node{}, c);", pid);
            pr("}");
        } else if (auto nn = dynamic_cast<node_node const*>(&nd)) {
            pr("if (obj->{}) {{", nn->var);
            pr("to_XML(node{}, obj->{});", pid, nn->var);
            pr("}");
        } else if (auto tn = dynamic_cast<tag_node const*>(&nd)) {
            (*this)(pid, *tn);
        } else {
            abort();
        }
    }

    void operator()(int pid, attr const& a) {
        auto const id = id_++;

        if (a.test) {
            pr("if (test(obj->{})) {{", a.var);
        }

        pr("auto attr{} = node{}.append_attribute(\"{}\");", id, pid, rename(a.var));
        pr("to_attr(attr{}, obj->{});", id, a.var);

        if (a.test) {
            pr("}");
        }
    }
};

template <class F>
void gen_router(std::vector<spec> const& specs, std::string_view kind, F filter) {
    pr("{");

    for (auto const& spec : specs) {
        if (filter(spec)) {
            pr("if (auto obj2 = {}::dyncast(obj)) {{", spec.name);
            pr("to_XML(node, obj2);");
            pr("return;}");
        }
    }
    sep();

    pr(R"(auto errmsg = std::string("Unknown {}: ");)", kind);
    pr("if (obj) {");
    pr("errmsg += obj->node_name();");
    pr("} else {");
    pr(R"#(errmsg += "(null)";)#");
    pr("}");
    pr("throw std::runtime_error(errmsg);");

    pr("}");
}

void gen_router(std::vector<spec> const& specs) {
    pr("void to_XML(pugi::xml_node& node, std::shared_ptr<xcml::node> obj)");
    gen_router(specs, "node", [](spec const&) {
        return true;
    });
    sep();

    pr("void to_XML(pugi::xml_node& node, std::shared_ptr<type_node> obj)");
    gen_router(specs, "type", [](spec const& spec) {
        return spec.is_type();
    });
    sep();

    pr("void to_XML(pugi::xml_node& node, std::shared_ptr<decl_node> obj)");
    gen_router(specs, "decl", [](spec const& spec) {
        return spec.is_decl();
    });
    sep();

    pr("void to_XML(pugi::xml_node& node, std::shared_ptr<stmt_node> obj)");
    gen_router(specs, "stmt", [](spec const& spec) {
        return spec.is_stmt();
    });
    sep();

    pr("void to_XML(pugi::xml_node& node, std::shared_ptr<expr_node> obj)");
    gen_router(specs, "expr", [](spec const& spec) {
        return spec.is_expr() || spec.is_unary() || spec.is_binary();
    });
    sep();
}

void gen_to_xml(xml_spec const& spec) {
    auto const& top = spec.top;
    auto const& ty = top->var;

    pr("void to_XML(pugi::xml_node node0, [[maybe_unused]] std::shared_ptr<{}> obj) {{", ty);
    generator()(*top);
    pr("}");
    sep();
}

}  // namespace

int main(int argc, char** argv) {
    auto specs = load_specs();
    auto xml_specs = load_xml_specs();
    g_tagmap = make_tag_map(specs);

    pr(R"(
        #include <pugixml.hpp>
        #include "xcml_type.hpp"
        #include "xcml_to_xml.hpp"
        
        using namespace xcml;

        namespace {
            bool test(bool b);

            bool test(storage_class sc);

            bool test(ref_scope r);

            bool test(std::string const& s);

            bool test(std::shared_ptr<compound_stmt> stmt);

            template <class Node>
            bool test(std::vector<std::shared_ptr<Node>> const& vec);

            void to_attr(pugi::xml_attribute& attr, bool b);

            void to_attr(pugi::xml_attribute& attr, size_t n);

            void to_attr(pugi::xml_attribute& attr, std::string const& str);

            void to_attr(pugi::xml_attribute& attr, storage_class sc);

            void to_attr(pugi::xml_attribute& attr, ref_scope rs);

            void to_XML(pugi::xml_node& node, size_t v);

            void to_XML(pugi::xml_node& node, std::string const& s);

            void to_XML(pugi::xml_node& node, std::shared_ptr<xcml::node> n);

            void to_XML(pugi::xml_node& node, std::shared_ptr<type_node> decl);

            void to_XML(pugi::xml_node& node, std::shared_ptr<decl_node> decl);

            void to_XML(pugi::xml_node& node, std::shared_ptr<stmt_node> stmt);

            void to_XML(pugi::xml_node& node, std::shared_ptr<expr_node> expr);
        }

        namespace xcml {
            void to_xml(pugi::xml_node& xml, node_ptr const& node) {
                to_XML(xml, node);
            }

            void to_xml(pugi::xml_node& xml, xcml_program_node_ptr const& node) {
                to_XML(xml, node);
            }
        }

        namespace {
    )");

    for (auto const& spec : xml_specs) {
        gen_to_xml(spec);
    }
    gen_router(specs);

    pr("void to_attr(pugi::xml_attribute& attr, storage_class sc) {");
    pr("switch (sc) {");
    for (auto const& v : {"none", "auto_", "param", "extern_", "extern_def", "static_",
                          "register_", "label_", "tagname", "moe", "typedef_name"}) {
        pr("case storage_class::{}: attr = \"{}\"; break;", v, rename(v));
    }
    pr("}");
    pr("}");
    sep();

    pr("void to_attr(pugi::xml_attribute& attr, ref_scope rs) {");
    pr("switch (rs) {");
    for (auto const& v : {"none", "global", "local", "param"}) {
        pr("case ref_scope::{}: attr = \"{}\"; break;", v, rename(v));
    }
    pr("}");
    pr("}");
    sep();

    pr(R"(
        bool test(bool b)
        { return b; }

        bool test(storage_class sc)
        { return sc != storage_class::none; }

        bool test(ref_scope sc)
        { return sc != ref_scope::none; }

        bool test(std::string const& s)
        { return !s.empty(); }

        bool test(std::shared_ptr<compound_stmt> stmt)
        { return stmt != nullptr && !stmt->body.empty(); }

        template <class Node> bool test(std::vector<std::shared_ptr<Node>> const& vec)
        { return !vec.empty(); }

        void to_attr(pugi::xml_attribute& attr, bool b)
        { attr = b ? "1" : "0"; }

        void to_attr(pugi::xml_attribute& attr, size_t n)
        { attr = std::to_string(n).c_str(); }

        void to_attr(pugi::xml_attribute& attr, std::string const& str)
        { attr = str.c_str(); }

        void to_XML(pugi::xml_node& node, size_t val) {
            auto node1 = node.append_child("uint64");
            to_XML(node1, std::to_string(val)); 
        }
                
        void to_XML(pugi::xml_node& node, std::string const& s)
        { node.text().set(s.c_str()); }
                
        }
    )");

    if (argc == 2) {
        std::ofstream ofs(argv[1]);
        ofs.write(buffer.data(), buffer.size());
    } else {
        std::cout.write(buffer.data(), buffer.size());
    }

    return 0;
}
