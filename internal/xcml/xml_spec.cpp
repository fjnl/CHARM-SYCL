#include <fmt/format.h>
#include <peglib.h>
#include "spec.hpp"

namespace {

static char const* XML_GRAMMAR = R"(
    TopLevel <- Spec*
    Spec <- '(' Name '(' Attr* ')' SpecBody* ')'
    Name <- !'.' Symbol
    Attr <- '(' Name '.test' ')'
          / '(' Name ')'
    SpecBody <- '(' '.for-each' Name ')'
              / '(' '.nodes' Name ')'
              / '(' '.node' Name ')'
              / '(' '.if' Name SpecBody* ')'
              / Spec
              / Name
    Symbol <- < [^ () \r\n\t\v]+ >
    %whitespace <- [ \r\n]*
)";

static char const* XML_SPEC = R"#(
(symbol_id ((type) (sclass .test))
    (name () name)
    (.if gccAttributes
        (gccAttributes ()
            (.for-each gccAttributes)))
    (.if cudaAttributes
        (cudaAttributes ()
            (.for-each cudaAttributes))))
(pragma ()
    value)
(compound_stmt ()
    (symbols ()
        (.for-each symbols))
    (declarations ()
        (.for-each declarations))
    (body ()
        (.for-each body)))
(gcc_attribute ((name)))
(cuda_attribute ()
    value)
(for_stmt ()
    (pragma ()
        (.for-each pragma))
    (init ()
        (.node init))
    (condition ()
        (.node condition))
    (iter ()
        (.node iter))
    (body ()
        (.node body)))
(if_stmt ()
    (condition ()
        (.node condition))
    (then ()
        (.node then))
    (.if else_
        (else_ ()
            (.node else_))))
(switch_stmt ()
    (value ()
        (.node value))
    (body ()
        (.node body)))
(case_label_stmt ()
    (value ()
        (.node value)))
(default_stmt ())
(return_stmt ()
    (.node value))
(expr_stmt ()
    (.node expr))
(parallel_invoke ()
    (dimensions ()
        (.for-each dimensions))
    (function ()
        (.node function))
    (arguments ()
        (.for-each arguments)))
(ndr_invoke ()
    (group ()
        (.for-each group))
    (local ()
        (.for-each local))
    (function ()
        (.node function))
    (arguments ()
        (.for-each arguments)))
(dimension ((iter) (type))
    (offset ()
        (.node offset))
    (size ()
        (.node size)))
(member_ref ((type .test) (member))
    (.node value))
(var_ref ((type .test) (scope .test))
    name)
(int_constant ((type))
    value)
(long_long_constant ((type))
    value)
(string_constant ()
    value)
(float_constant ((type))
    value)
(cast_expr ((type) (to_base .test))
    (.node value))
(cond_expr ()
    (.node cond)
    (.node true_)
    (.node false_))
(func_addr ((type .test))
    name)
(var_addr ((scope .test) (type .test))
    name)
(array_addr ((scope .test) (type .test))
    name)
(member_addr ((type .test) (member))
    (.node value))
(pointer_ref ((type .test))
    (.node expr))
(array_ref ((type .test))
    (.node array)
    (.nodes index))
(member_array_ref ((type .test) (member))
    (.node value))
(function_call ((type .test))
    (function () 
        (.node function))
    (arguments ()
        (.for-each arguments)))
(break_stmt ())
(continue_stmt ())
(param_node ((type))
    name)
(ellipsis ())
(basic_type ((type) (is_const) (name) (is_builtin .test)))
(pointer_type ((type) (ref)))
(function_type ((type) (return_type))
    (params ()
        (.for-each params))
    (cudaAttributes ()
        (.for-each cuda_attrs)))
(struct_type ((type))
    (symbols ()
        (.for-each symbols)))
(array_type ((type) (element_type) (array_size)))
(function_definition ()
    (gccAttributes ()
        (.for-each attributes))
    (name () name)
    (symbols ()
        (.for-each symbols))
    (params ()
        (.for-each params))
    (body ()
        (.node body)))
(var_decl ()
    (name () name))
(function_decl ((extern_c .test) (inline_ .test) (force_inline .test))
    (name () name))
(runtime_func_decl ()
    (name () name)
    (func_kind () func_kind))
(kernel_wrapper_decl ((name))
    (symbols ()
        (.for-each symbols))
    (params ()
        (.for-each params))
    (memories ()
        (.for-each memories))
    (body ()
        (.node body)))
(cpp_include ()
    (name () name))
(xcml_program_node ((language .test) (gensym_id .test))
    (.if extra
        (extra () extra))
    (typeTable ()
        (.for-each type_table))
    (globalSymbols ()
        (.for-each global_symbols))
    (globalDeclarations ()
        (.for-each global_declarations)))
)#";

struct op_base {
    std::string_view _name;
    std::string_view op;

    std::string name() const {
        for (std::string_view pre : {"asg", "log", "bit", "post", "pre", "addr", "unary"}) {
            auto const n = pre.size();
            if (_name.substr(0, n) == pre) {
                std::string res;
                res.append(pre);
                res.append("_");
                res.append(to_lower(_name.substr(n)));
                res.append("_expr");
                return res;
            }
        }

        return std::string(to_lower(_name)) + "_expr";
    }

protected:
    static std::string to_lower(std::string_view sv) {
        std::string res;
        std::transform(sv.begin(), sv.end(), std::back_inserter(res), &tolower);
        return res;
    }
};

struct bin_op : op_base {
} const BIN_OPS[] = {
    {"Lshift", "<<"},     {"Rshift", ">>"},    {"plus", "+"},      {"minus", "-"},
    {"mul", "*"},         {"div", "/"},        {"mod", "%"},       {"logEQ", "=="},
    {"logNEQ", "!="},     {"logGE", ">="},     {"logGT", ">"},     {"logLE", "<="},
    {"logLT", "<"},       {"logAnd", "&&"},    {"logOr", "||"},    {"bitAnd", "&"},
    {"bitOr", "|"},       {"bitXor", "^"},     {"assign", "="},    {"asgPlus", "+="},
    {"asgMinus", "-="},   {"asgMul", "*="},    {"asgDiv", "/="},   {"asgLshift", "<<="},
    {"asgRshift", ">>="}, {"asgBitAnd", "&="}, {"asgBitOr", "|="}, {"asgBitXor", "^="},
};

struct unary_op : op_base {
} const UNARY_OPS[] = {{"logNot", "!"},    {"bitNot", "~"},     {"preIncr", "++"},
                       {"postIncr", "++"}, {"preDecr", "--"},   {"postDecr", "--"},
                       {"addrOf", "&"},    {"unaryMinus", "-"}, {"unaryPlus", "+"}};

struct xml_spec_parser {
    std::vector<xml_spec> parse() {
        std::string spec;
        spec.append(XML_SPEC);
        spec.append("\n");

        for (auto const& op : UNARY_OPS) {
            spec += fmt::format("({} ((type .test)) (.node expr))", op.name());
        }

        for (auto const& op : BIN_OPS) {
            spec += fmt::format("({} ((type .test)) (.node lhs) (.node rhs))", op.name());
        }

        return parse(spec);
    }

private:
    std::vector<xml_spec> parse(std::string_view input) {
        peg::parser g;
        g.set_logger([](size_t line, size_t col, std::string const& msg) {
            std::cerr << line << ':' << col << ": error: " << msg << std::endl;
        });

        if (!g.load_grammar(XML_GRAMMAR)) {
            exit(1);
        }

        g.enable_ast();
        g.enable_packrat_parsing();
        // peg::enable_tracing(g, std::cerr);

        std::shared_ptr<peg::Ast> ast;

        if (!g.parse(input, ast)) {
            exit(1);
        }

        return visit_(ast);
    }

    std::vector<xml_spec> visit_(std::shared_ptr<peg::Ast> ast) {
        if (ast->name == "TopLevel") {
            std::vector<xml_spec> specs;
            for (auto const& c : ast->nodes) {
                visit_(specs, c);
            }
            return specs;
        }

        fmt::print(stderr, "Error: {} in {}\n", ast->name, __PRETTY_FUNCTION__);
        abort();
    }

    void visit_(std::vector<xml_spec>& specs, std::shared_ptr<peg::Ast> ast) {
        if (ast->name == "Spec") {
            xml_spec spec;
            spec.top = std::make_unique<tag_node>();

            auto it = ast->nodes.begin();

            visit_(spec.top->var, *it);

            for (++it; it != ast->nodes.end(); ++it) {
                if ((*it)->name != "Attr") {
                    break;
                }
                visit_(spec.top->attrs, *it);
            }

            for (; it != ast->nodes.end(); ++it) {
                visit_(spec.top->body, *it);
            }

            specs.push_back(std::move(spec));
            return;
        }

        fmt::print(stderr, "Error: {} in {}\n", ast->name, __PRETTY_FUNCTION__);
        abort();
    }

    void visit_(std::vector<attr>& attrs, std::shared_ptr<peg::Ast> ast) {
        if (ast->name == "Attr") {
            attr x;
            visit_(x.var, ast->nodes.at(0));
            x.test = (ast->choice == 0);

            attrs.push_back(x);

            return;
        }

        fmt::print(stderr, "Error: {} in {}\n", ast->name, __PRETTY_FUNCTION__);
        abort();
    }

    void visit_(std::vector<std::unique_ptr<xml_node>>& nodes, std::shared_ptr<peg::Ast> ast) {
        if (ast->name == "SpecBody") {
            switch (ast->choice) {
                case 0: {
                    auto nd = std::make_unique<foreach_node>();
                    visit_(nd->var, ast->nodes.at(0));
                    nodes.push_back(std::move(nd));
                    break;
                }

                case 1: {
                    auto nd = std::make_unique<nodes_node>();
                    visit_(nd->var, ast->nodes.at(0));
                    nodes.push_back(std::move(nd));
                    break;
                }

                case 2: {
                    auto nd = std::make_unique<node_node>();
                    visit_(nd->var, ast->nodes.at(0));
                    nodes.push_back(std::move(nd));
                    break;
                }

                case 3: {
                    auto nd = std::make_unique<if_node>();
                    visit_(nd->var, ast->nodes.at(0));
                    for (size_t i = 1; i < ast->nodes.size(); i++) {
                        visit_(nd->body, ast->nodes.at(i));
                    }
                    nodes.push_back(std::move(nd));
                    break;
                }

                case 4: {
                    visit_(nodes, ast->nodes.at(0));
                    break;
                }

                default: {
                    auto nd = std::make_unique<ref_node>();
                    visit_(nd->var, ast->nodes.at(0));
                    nodes.push_back(std::move(nd));
                    break;
                }
            }

            return;
        }

        if (ast->name == "Spec") {
            std::vector<xml_spec> specs;
            visit_(specs, ast);

            nodes.push_back(std::move(specs.front().top));

            return;
        }

        fmt::print(stderr, "Error: {} in {}\n", ast->name, __PRETTY_FUNCTION__);
        abort();
    }

    void visit_(std::string& str, std::shared_ptr<peg::Ast> ast) {
        if (ast->name == "Symbol") {
            str = ast->token;
            return;
        }
        if (ast->name == "Name") {
            visit_(str, ast->nodes.at(0));
            return;
        }

        fmt::print(stderr, "Error: {} in {}\n", ast->name, __PRETTY_FUNCTION__);
        abort();
    }
};

}  // namespace

std::vector<xml_spec> load_xml_specs() {
    return xml_spec_parser().parse();
}
