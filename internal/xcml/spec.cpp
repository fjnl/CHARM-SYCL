#include "spec.hpp"
#include <variant>
#include <fmt/format.h>
#include <peglib.h>

namespace {

static char const* SPEC = R"#(
    (N symbol_id             id                 () ((sclass sclass) (string type) (string name) (expr value) (attr* gccAttributes) (cuda_attribute* cudaAttributes)))
    (N pragma                pragma             () ((string value)))
    (S compound_stmt         compoundStatement  () ((symbol% symbols) (decl% declarations) (stmt% body)))
    (N gcc_attribute         gccAttribute       () ((string name)))
    (N cuda_attribute        cudaAttribute      () ((string value)))
    (S for_stmt              forStatement       () ((expr init) (expr condition) (expr iter) (compound body) (pragma* pragma)))
    (S if_stmt               ifStatement        () ((expr condition) (compound then) (compound else_)))
    (S switch_stmt           switchStatement    () ((expr value) (compound body)))
    (S case_label_stmt       caseLabelStatement () ((expr value)))
    (S default_stmt       defaultLabelStatement () ())
    (S return_stmt           returnStatement    () ((expr value)))
    (S expr_stmt             exprStatement      () ((expr expr)))
    (S parallel_invoke       parallelInvoke     () ((dimension* dimensions) (expr function) (expr* arguments)))
    (S ndr_invoke            ndrInvoke          () ((dimension* group) (dimension* local) (expr function) (expr* arguments)))
    (S break_stmt            breakStatement     () ())
    (S continue_stmt         continueStatement  () ())
    (N dimension             dimension          () ((string iter) (string type) (expr offset) (expr size)))
    (E var_ref               Var                () ((string name) (ref_scope scope)))
    (E member_ref            memberRef          () ((string member) (expr value)))
    (E func_addr             funcAddr           () ((string name)))
    (E var_addr              varAddr            () ((string name) (ref_scope scope)))
    (E array_addr            arrayAddr          () ((string name) (ref_scope scope)))
    (E member_addr           memberAddr         () ((string member) (expr value)))
    (E pointer_ref           pointerRef         () ((expr expr)))
    (E array_ref             arrayRef           () ((expr array) (expr* index)))
    (E member_array_ref      memberArrayRef     () ((string member) (expr value)))
    (E function_call         functionCall       () ((expr function) (expr* arguments)))
    (E int_constant          intConstant        () ((string value)))
    (E long_long_constant    longlongConstant   () ((string value)))
    (E string_constant       stringConstant     () ((string value)))
    (E float_constant        floatConstant      () ((string value)))
    (E cast_expr             castExpr           () ((expr value) (bool to_base)))
    (E cond_expr             condExpr           () ((expr cond) (expr true_) (expr false_)))
    (P param_node            name               () ((string name) (string type)))
    (P ellipsis              ellipsis           () ())
    (T basic_type            basicType          () ((string name) (bool is_const) (bool is_builtin)))
    (T pointer_type          pointerType        () ((string ref) (bool is_restrict)))
    (T function_type         functionType       () ((string return_type) (param* params) (cuda_attribute* cuda_attrs)))
    (T struct_type           structType         () ((symbol* symbols)))
    (T array_type            arrayType          () ((string element_type) (size_t array_size)))
    (D function_definition   functionDefinition () ((symbol* symbols) (param* params) (compound body) (attr* attributes)))
    (D var_decl              varDecl            () ())
    (D function_decl         functionDecl       () ((bool extern_c) (bool inline_) (bool force_inline)))
    (D runtime_func_decl     runtimeFuncDecl    () ((string name) (string func_kind)))
    (D kernel_wrapper_decl   kernelWrapperDecl  () ((symbol* symbols) (param* params) (size_t# memories) (stmt body)))
    (D cpp_include           cppInclude         () ())
    (N xcml_program_node     XcodeProgram       () ((type% type_table) (symbol% global_symbols) (decl% global_declarations) (string extra) (string language) (size_t gensym_id)))
)#";

static char const* GRAMMAR = R"(
    TopLevel <- Spec*
    Spec <- '(' Type Name Tag Op '(' Member* ')' ')'
    Type <- 'N' / 'S' / 'E' / 'D' / 'T' / 'B' / 'U' / 'P'
    Name <- Symbol
    Tag <- Symbol
    Op <- '(' ')' / Symbol
    Symbol <- < [^ () \r\n\t\v]+ >
    Member <- '(' Name Name ')'
    %whitespace <- [ \r\n]*
)";

using str_pair = std::pair<std::string, std::string>;

struct {
    std::string_view name;
    std::string_view op;
} const BIN_OPS[] = {
    {"Lshift", "<<"},     {"Rshift", ">>"},    {"plus", "+"},      {"minus", "-"},
    {"mul", "*"},         {"div", "/"},        {"mod", "%"},       {"logEQ", "=="},
    {"logNEQ", "!="},     {"logGE", ">="},     {"logGT", ">"},     {"logLE", "<="},
    {"logLT", "<"},       {"logAnd", "&&"},    {"logOr", "||"},    {"bitAnd", "&"},
    {"bitOr", "|"},       {"bitXor", "^"},     {"assign", "="},    {"asgPlus", "+="},
    {"asgMinus", "-="},   {"asgMul", "*="},    {"asgDiv", "/="},   {"asgLshift", "<<="},
    {"asgRshift", ">>="}, {"asgBitAnd", "&="}, {"asgBitOr", "|="}, {"asgBitXor", "^="},
};

struct {
    std::string_view name;
    std::string_view op;
} const UNARY_OPS[] = {{"logNot", "!"},    {"bitNot", "~"},     {"preIncr", "++"},
                       {"postIncr", "++"}, {"preDecr", "--"},   {"postDecr", "--"},
                       {"addrOf", "&"},    {"unaryMinus", "-"}, {"unaryPlus", "+"}};

struct spec_parser {
    std::vector<spec> parse() {
        std::string data(SPEC);
        auto out = std::back_inserter(data);

#define PREFIX(pre)                           \
    if (starts_with(op.name, #pre)) {         \
        name = #pre;                          \
        name += '_';                          \
        name += op.name.substr(strlen(#pre)); \
        name += "_expr";                      \
    }

        for (auto const& op : BIN_OPS) {
            auto name = std::string(op.name) + "_expr";
            PREFIX(asg);
            PREFIX(log);
            PREFIX(bit);
            PREFIX(post);
            PREFIX(pre);
            PREFIX(addr);
            PREFIX(unary);
            fmt::format_to(out, "(B {} {}Expr {} ())\n", to_lower(name), op.name, op.op);
        }

        for (auto const& op : UNARY_OPS) {
            auto name = std::string(op.name) + "_expr";
            PREFIX(asg);
            PREFIX(log);
            PREFIX(bit);
            PREFIX(post);
            PREFIX(pre);
            PREFIX(addr);
            PREFIX(unary);
            fmt::format_to(out, "(U {} {}Expr {} ())\n", to_lower(name), op.name, op.op);
        }

        return parse(data);
    }

    bool starts_with(std::string_view str, std::string_view prefix) const {
        if (str.size() < prefix.size()) {
            return false;
        }
        return std::equal(str.begin(), str.begin() + prefix.size(), prefix.begin());
    }

    std::string to_lower(std::string_view sv) const {
        std::string res;
        std::transform(sv.begin(), sv.end(), std::back_inserter(res), &tolower);
        return res;
    }

    std::vector<spec> parse(std::string_view input) {
        peg::parser g;
        g.set_logger([](size_t line, size_t col, std::string const& msg) {
            std::cerr << line << ':' << col << ": error: " << msg << std::endl;
        });

        if (!g.load_grammar(GRAMMAR)) {
            exit(1);
        }

        g.enable_ast();
        g.enable_packrat_parsing();

        std::shared_ptr<peg::Ast> ast;

        if (!g.parse(input, ast)) {
            exit(1);
        }

        std::vector<spec> specs;
        visit_(specs, ast);

        return specs;
    }

private:
    using result_t = std::variant<std::monostate, int, std::string_view, str_pair>;

    uint32_t next_id_ = 1;

    result_t visit_(std::vector<spec>& specs, std::shared_ptr<peg::Ast> ast) {
        if (ast->name == "TopLevel") {
            for (auto const& c : ast->nodes) {
                visit_(specs, c);
            }
            return {};
        }

        if (ast->name == "Spec") {
            std::vector<str_pair> ms;

            for (size_t i = 4; i < ast->nodes.size(); i++) {
                ms.push_back(visit<str_pair>(specs, ast->nodes.at(i)));
            }

            spec spc;
            spc.type = visit<int>(specs, ast->nodes.at(0));
            spc.name = visit<std::string_view>(specs, ast->nodes.at(1));
            spc.tag = visit<std::string_view>(specs, ast->nodes.at(2));
            spc.op = visit<std::string_view>(specs, ast->nodes.at(3));
            spc.members = std::move(ms);
            spc.id = next_id_++;

            specs.push_back(std::move(spc));

            return {};
        }

        if (ast->name == "Type") {
            return 1 << ast->choice;
        }

        if (ast->name == "Name") {
            return visit_(specs, ast->nodes.at(0));
        }

        if (ast->name == "Tag") {
            return visit_(specs, ast->nodes.at(0));
        }

        if (ast->name == "Op") {
            if (ast->choice == 0) {
                return std::string_view();
            }
            return visit_(specs, ast->nodes.at(0));
        }

        if (ast->name == "Symbol") {
            return ast->token;
        }

        if (ast->name == "Member") {
            return std::make_pair(visit<std::string>(specs, ast->nodes.at(0)),
                                  visit<std::string>(specs, ast->nodes.at(1)));
        }

        fmt::print(stderr, "Error: {} in {}\n", ast->name, __PRETTY_FUNCTION__);
        abort();
    }

    template <class T>
    T visit(std::vector<spec>& specs, std::shared_ptr<peg::Ast> ast) {
        if constexpr (std::is_same_v<T, std::string>) {
            return std::string(std::get<std::string_view>(visit_(specs, ast)));
        } else {
            return std::get<T>(visit_(specs, ast));
        }
    }
};

}  // namespace

std::vector<spec> load_specs() {
    return spec_parser().parse();
}
