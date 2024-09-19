#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
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

std::string_view rename(std::string_view name) {
    auto const len = name.size();
    if (!name.empty() && name.at(len - 1) == '_') {
        return name.substr(0, len - 1);
    }
    return name;
}

std::unordered_map<std::string, std::string> make_tag_map(std::vector<spec> const& specs) {
    std::unordered_map<std::string, std::string> map;

    for (auto const& s : specs) {
        map.insert_or_assign(s.name, s.tag);
    }
    return map;
}

static std::unordered_map<std::string, std::string> g_tagmap;

struct generator {
    void top(tag_node const& node) const {
        pr("assert(node0);");
        pr("assert(node0.name() == std::string_view(\"{}\"));", g_tagmap.at(node.var));
        sep();
        pr("if (obj == nullptr){{ obj = new_{}(); }}", node.var);
        sep();

        pr(R"(
            if (auto file = node0.attribute("file")) {
                obj->file = file.as_string();
            }
            if (auto line = node0.attribute("line")) {
                obj->line = line.as_int();
            }
        )");

        int next_id = 0;
        apply(node, 0, next_id, true);
    }

    void apply(xml_node const& fn, int pid, int& next_id, int& idx) const {
        if (auto x = dynamic_cast<foreach_node const*>(&fn)) {
            apply(*x, pid, next_id);
        } else if (auto x = dynamic_cast<nodes_node const*>(&fn)) {
            apply(*x, pid, next_id, idx);
        } else if (auto x = dynamic_cast<node_node const*>(&fn)) {
            apply(*x, pid, next_id, idx);
        } else if (auto x = dynamic_cast<ref_node const*>(&fn)) {
            apply(*x, pid, next_id);
        } else if (auto x = dynamic_cast<if_node const*>(&fn)) {
            apply(*x, pid, next_id);
        } else if (auto x = dynamic_cast<tag_node const*>(&fn)) {
            apply(*x, pid, next_id);
        } else {
            abort();
        }
    }

    void apply(foreach_node const& fn, int pid, int& next_id) const {
        auto const id = next_id++;

        pr("for (auto const& node{} : node{}.children()) {{", id, pid);
        pr("for_each(obj->{}, node{});", fn.var, id);
        pr("}");
    }

    void apply(ref_node const& rn, int pid, int&) const {
        pr("from_XML(node{}, obj->{});", pid, rn.var);
    }

    void apply(nodes_node const& nn, int pid, int& next_id, int& idx) const {
        auto const tid = next_id++;

        pr("for (auto it = std::next(node{}.begin(), {}); it != node{}.end(); ++it) {{", pid,
           idx, pid);
        pr("using type{} = decltype(obj->{})::value_type::element_type;", tid, nn.var);
        pr("obj->{}.push_back(make_obj<type{}>(*it));", nn.var, tid);
        pr("}");
        idx = -1;
    }

    void apply(node_node const& nn, int pid, int&, int& idx) const {
        pr("if (node{}.begin() != node{}.end()) {{", pid, pid);
        pr("from_XML(*std::next(node{}.begin(), {}), obj->{});", pid, idx, nn.var);
        pr("}");
        idx++;
    }

    void apply(if_node const& ifn, int pid, int& next_id) const {
        pr("if (!node{}.child(\"{}\").empty()) {{", pid, rename(ifn.var));

        int idx = 0;
        for (auto const& b : ifn.body) {
            apply(*b, pid, next_id, idx);
            sep();
        }

        pr("}");
    }

    void apply(tag_node const& tn, int pid, int& next_id, bool top = false) const {
        auto const id = next_id++;

        if (!top) {
            pr("auto node{} = node{}.child(\"{}\");", id, pid, rename(tn.var));
        }

        for (auto const& attr : tn.attrs) {
            auto const id = next_id++;
            pr("auto attr{} = node{}.attribute(\"{}\");", id, pid, rename(attr.var));

            if (attr.test) {
                pr("if (attr{}) {{", id);
            } else {
                pr("assert(attr{});", id);
            }

            pr("from_attr(attr{}, obj->{});", id, attr.var);

            if (attr.test) {
                pr("}");
            }
            sep();
        }

        int idx = 0;
        for (auto const& x : tn.body) {
            apply(*x, id, next_id, idx);
            sep();
        }
    }
};

void gen_from_xml(xml_spec const& spec, bool fwd) {
    pr("inline void from_XML([[maybe_unused]] pugi::xml_node node0, std::shared_ptr<{}>& obj)",
       spec.top->var);

    if (fwd) {
        pr(";");
    } else {
        pr("{");
        generator().top(*spec.top);
        pr("}");
    }

    sep();
}

void gen_factory(std::vector<spec> const& specs, std::vector<xml_spec> const& xml_specs) {
    pr("template <class T> inline "
       "std::conditional_t<std::is_integral_v<T>, T, std::shared_ptr<T>> "
       "make_obj(pugi::xml_node node) {");

    pr("if constexpr (std::is_integral_v<T>) {");
    pr("if (node.name() == std::string_view(\"uint64\")) {");
    pr("std::string v; from_XML(node, v); return strtoumax(v.c_str(), nullptr, 0);");
    pr("}");
    pr("else if (node.name() == std::string_view(\"int64\")) {");
    pr("std::string v; from_XML(node, v); return strtoimax(v.c_str(), nullptr, 0);");
    pr("}");
    pr("fprintf(stderr, \"%s is not an integer value\\n\", node.name());");
    pr("assert(false);");
    pr("return static_cast<T>(-1);");
    pr("}");

    pr("else if constexpr (std::is_same_v<T, type_node>) {");
    pr("std::shared_ptr<type_node> ret;");
    for (auto const& spec : specs) {
        if (spec.is_type()) {
            pr("if (node.name() == std::string_view(\"{}\")) {{", spec.tag);
            pr("ret = make_obj<{}>(node);", spec.name);
            pr("}}", spec.tag);
        }
    }
    pr("if(!ret){ fprintf(stderr, \"%s is not a type_node\\n\", node.name()); }");
    pr("assert(ret); return ret; }");

    pr("else if constexpr (std::is_same_v<T, decl_node>) {");
    pr("std::shared_ptr<decl_node> ret;");
    for (auto const& spec : specs) {
        if (spec.is_decl()) {
            pr("if (node.name() == std::string_view(\"{}\")) {{", spec.tag);
            pr("ret = make_obj<{}>(node);", spec.name);
            pr("}}", spec.tag);
        }
    }
    pr("if(!ret){ fprintf(stderr, \"%s is not a decl_node\\n\", node.name()); }");
    pr("assert(ret); return ret; }");

    pr("else if constexpr (std::is_same_v<T, stmt_node>) {");
    pr("std::shared_ptr<stmt_node> ret;");
    for (auto const& spec : specs) {
        if (spec.is_stmt()) {
            pr("if (node.name() == std::string_view(\"{}\")) {{", spec.tag);
            pr("ret = make_obj<{}>(node);", spec.name);
            pr("}}", spec.tag);
        }
    }
    pr("if(!ret){ fprintf(stderr, \"%s is not a stmt_node\\n\", node.name()); }");
    pr("assert(ret); return ret; }");

    pr("else if constexpr (std::is_same_v<T, expr_node>) {");
    pr("std::shared_ptr<expr_node> ret;");
    for (auto const& spec : specs) {
        if (spec.is_expr() || spec.is_unary() || spec.is_binary()) {
            pr("if (node.name() == std::string_view(\"{}\")) {{", spec.tag);
            pr("ret = make_obj<{}>(node);", spec.name);
            pr("}}", spec.tag);
        }
    }
    pr("if(!ret){ fprintf(stderr, \"%s is not a expr_node\\n\", node.name()); }");
    pr("assert(ret); return ret; }");

    pr("else if constexpr (std::is_same_v<T, params_node>) {");
    pr("std::shared_ptr<params_node> ret;");
    for (auto const& spec : specs) {
        if (spec.is_param()) {
            pr("if (node.name() == std::string_view(\"{}\")) {{", spec.tag);
            pr("ret = make_obj<{}>(node);", spec.name);
            pr("}}", spec.tag);
        }
    }
    pr("if(!ret){ fprintf(stderr, \"%s is not a params_node\\n\", node.name()); }");
    pr("assert(ret); return ret; }");

    for (size_t i = 0; i < xml_specs.size(); i++) {
        auto const& spec = xml_specs.at(i);
        auto const& ty = spec.top->var;

        if (ty == "xcml_program_node") {
            continue;
        }

        pr("else if constexpr (std::is_same_v<T, {}>) {{", ty);
        pr("std::shared_ptr<{}> obj = new_{}();", ty, ty);
        pr("from_XML(node,  obj);");
        pr("return obj;");
        pr("}");
    }

    pr("}");
    sep();
}

}  // namespace

int main(int argc, char** argv) {
    auto specs = load_specs();
    auto xml_specs = load_xml_specs();

    g_tagmap = make_tag_map(specs);

    pr(R"(
        #include <cassert>
        #include <inttypes.h>
        #include <pugixml.hpp>
        #include <utility>
        #include "xcml_type.hpp"
        #include "xcml_to_xml.hpp"
        
        using namespace xcml;

        namespace {

        template <class T>
        std::conditional_t<std::is_integral_v<T>, T, std::shared_ptr<T>>
        make_obj(pugi::xml_node node);

        void from_attr(pugi::xml_attribute attr, bool& v)
        { v = std::string_view(attr.value()) == "1"; }
    
        void from_attr(pugi::xml_attribute attr, size_t& v)
        { v = std::stoull(attr.value()); }

        void from_attr(pugi::xml_attribute attr, std::string& v)
        { v = attr.value(); }
    
        void from_XML(pugi::xml_node& node, std::string& obj)
        { obj = node.text().get(); }

        void from_XML(pugi::xml_node& node, std::shared_ptr<expr_node>& obj);

        // void from_XML(pugi::xml_node& node, std::shared_ptr<stmt_node>& obj);

        template <class T>
        void for_each(std::vector<std::shared_ptr<T>>& vec, pugi::xml_node const& node);

        template <class T>
        void for_each(std::list<std::shared_ptr<T>>& list, pugi::xml_node const& node);

        template <class T> void for_each(std::set<T>& set, pugi::xml_node const& node);
    )");

    pr("void from_attr(pugi::xml_attribute attr, storage_class& v)");
    pr("{ auto const attrval = std::string(attr.value());");
    for (auto const& v : {"none", "auto_", "param", "extern_", "extern_def", "static_",
                          "register_", "label_", "tagname", "moe", "typedef_name"}) {
        pr("if (attrval == \"{}\") {{", rename(v));
        pr("v = storage_class::{};", v);
        pr("return;");
        pr("}");
    }
    pr("}");
    sep();

    pr("void from_attr(pugi::xml_attribute attr, ref_scope& v)");
    pr("{ auto const attrval = std::string(attr.value());");
    for (auto const& v : {"none", "global", "local", "param"}) {
        pr("if (attrval == \"{}\") {{", rename(v));
        pr("v = ref_scope::{};", v);
        pr("return;");
        pr("}");
    }
    pr("}");
    sep();

    for (auto const& spec : xml_specs) {
        gen_from_xml(spec, true);
    }

    for (auto const& spec : xml_specs) {
        gen_from_xml(spec, false);
    }

    pr(R"(
        void from_XML(pugi::xml_node& node, std::shared_ptr<expr_node>& obj)
        { obj = make_obj<expr_node>(node); }

        // void from_XML(pugi::xml_node& node, std::shared_ptr<stmt_node>& obj)
        // { obj = make_obj<stmt_node>(node); }
    )");

    gen_factory(specs, xml_specs);

    pr(R"(
        template <class T>
        void for_each(std::vector<std::shared_ptr<T>>& vec, pugi::xml_node const& node)
        { vec.push_back(make_obj<T>(node)); }
    
        template <class T>
       void for_each(std::list<std::shared_ptr<T>>& list, pugi::xml_node const& node)
        { list.push_back(make_obj<T>(node)); }
    
        template <class T>
        void for_each(std::set<T>& set, pugi::xml_node const& node)
        { set.insert(make_obj<T>(node)); }
        
        }

        namespace xcml {

        void from_xml(pugi::xml_node const& xml, xcml::xcml_program_node_ptr& node) {
            from_XML(xml, node);
        }
        
        void make_from_xml(pugi::xml_node const& xml, xcml::expr_ptr& node) {
            node = make_obj<xcml::expr_node>(xml);
        }

        void make_from_xml(pugi::xml_node const& xml, xcml::compound_stmt_ptr& node) {
            node = make_obj<xcml::compound_stmt>(xml);
        }

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
