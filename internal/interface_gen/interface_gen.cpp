#include <format>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <vector>
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/BLAKE3.h>
#include <pugixml.hpp>

namespace {

std::vector<char> g_buffer;

template <class... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
    std::format_to(std::back_inserter(g_buffer), fmt, std::forward<Args>(args)...);
    g_buffer.push_back('\n');
}

struct Visitor : clang::RecursiveASTVisitor<Visitor> {
    explicit Visitor(clang::ASTContext& ctx) : ctx_(ctx) {}

    bool shouldVisitTemplateInstantiations() const {
        return true;
    }

    bool VisitCallExpr(clang::CallExpr const* expr) {
        if (auto const* callee = expr->getCalleeDecl()) {
            if (auto const* decl = callee->getAsFunction()) {
                if (decl->getName() == "begin_interfaces") {
                    begin_interfaces(decl);
                } else if (decl->getName() == "end_interfaces") {
                    end_interfaces();
                } else if (decl->getName() == "type_interface") {
                    type_interface(expr, decl);
                } else if (decl->getName() == "func_interface") {
                    func_interface(expr, decl);
                } else if (decl->getName() == "constant_interface") {
                    constant_interface(expr, decl);
                }
            }
        }

        return true;
    }

private:
    void begin_interfaces(clang::FunctionDecl const*) {
        print(R"(<interfaces>)");
        // print(R"(<from version="{}" />)",
        //       decl->getTemplateSpecializationArgs()->get(0).getAsIntegral().getExtValue());
    }

    void end_interfaces() {
        print("</interfaces>\n");
    }

    std::string_view strlit(clang::CallExpr const* call, int arg) {
        auto const* lit =
            clang::dyn_cast<clang::StringLiteral>(call->getArg(arg)->IgnoreCasts());
        return std::string_view(lit->getString());
    }

    void constant_interface(clang::CallExpr const* call, clang::FunctionDecl const* decl) {
        auto type = decl->getParamDecl(0)->getType();
        auto name = strlit(call, 1);

        auto const* arg0 = call->getArg(0);
        llvm::SmallVector<char> val;

        if (auto const* dr = clang::dyn_cast<clang::DeclRefExpr>(arg0->IgnoreCasts())) {
            if (auto const* ecd = clang::dyn_cast<clang::EnumConstantDecl>(dr->getDecl())) {
                ecd->getInitVal().toString(val);
            }
        } else {
            clang::Expr::EvalResult res;
            if (arg0->EvaluateAsInt(res, ctx_)) {
                res.Val.getInt().toString(val);
            }
        }

        if (val.empty()) {
            call->dumpColor();
            std::abort();
        }

        std::string type_name;
        if (type->isBuiltinType()) {
            auto type2 = ctx_.getCanonicalType(type);
            if (type2 == ctx_.IntTy) {
                type_name = "int";
            }
        } else {
            type_name = aliases_[ctx_.getCanonicalType(type)];
        }

        if (type_name.empty()) {
            type.dump();
            std::abort();
        }

        print(R"(<constant type="{}" name="{}" value="{}" />)", type_name, name,
              std::string_view(val.data(), val.size()));
    }

    std::map<clang::QualType, std::string> aliases_;

    void type_interface(clang::CallExpr const* call, clang::FunctionDecl const*) {
        auto type = clang::dyn_cast<clang::DeclRefExpr>(call->getArg(0)->IgnoreCasts())
                        ->getDecl()
                        ->getAsFunction()
                        ->getParamDecl(0)
                        ->getType();
        auto name = strlit(call, 1);
        auto alias = strlit(call, 2);

        aliases_.insert_or_assign(ctx_.getCanonicalType(type), alias);

        print(R"(<type name="{}" alias="{}">)", name, alias);
        dump_type(type, true);
        print(R"(</type>)");
    }

    void func_interface(clang::CallExpr const* call, clang::FunctionDecl const*) {
        auto const* fn = clang::dyn_cast<clang::FunctionDecl>(
            clang::dyn_cast<clang::DeclRefExpr>(call->getArg(0)->IgnoreCasts())->getDecl());
        auto name = strlit(call, 1);
        auto realname = strlit(call, 2);

        print(R"(<function name="{}" realname="{}">)", name, realname);

        print(R"(<return>)");
        dump_type(fn->getReturnType(), false);
        print(R"(</return>)");

        size_t idx = 0;
        for (auto const* param : fn->parameters()) {
            print(R"(<parameter index="{}">)", idx);
            dump_type(param->getType(), false);
            print(R"(</parameter>)");

            ++idx;
        }

        print(R"(</function>)");
    }

    void dump_type(clang::QualType type, bool verbose) {
        if (type->isTypedefNameType()) {
            auto et = clang::dyn_cast<clang::ElaboratedType>(type);
            auto tt = clang::dyn_cast<clang::TypedefType>(et->getNamedType());
            auto const* decl = tt->getDecl();

            if (verbose) {
                print(R"(<typedef name="{}">)", std::string_view(decl->getName()));
                dump_type(decl->getUnderlyingType(), verbose);
                print("</typedef>");
            } else {
                print(R"(<typedef name="{}" />)", std::string_view(decl->getName()));
            }
        } else if (type->isEnumeralType()) {
            auto const* decl =
                clang::dyn_cast<clang::EnumDecl>(type->getAsTagDecl()->getDefinition());
            auto int_type = decl->getIntegerType();

            print(R"(<enum name="{}" size="{}" signed="{}" />)",
                  std::string_view(decl->getName()), ctx_.getTypeSize(int_type),
                  int_type->isSignedIntegerType());
            // if (verbose) {
            //     for (auto const* e : decl->enumerators()) {
            //         llvm::SmallVector<char> val;
            //         if (int_type->isSignedIntegerType()) {
            //             e->getInitVal().toStringSigned(val);
            //         } else {
            //             e->getInitVal().toStringUnsigned(val);
            //         }

            //         print(R"(<enumerator name="{}" value="{}" />)",
            //               std::string_view(e->getName()),
            //               std::string_view(val.data(), val.size()));
            //     }
            // }
        } else if (type->isIntegerType()) {
            print(R"(<integer size="{}" signed="{}" />)", ctx_.getTypeSize(type),
                  type->isSignedIntegerType());
        } else if (type->isFloatingType()) {
            print(R"(<floating size="{}" />)", ctx_.getTypeSize(type));
        } else if (type->isPointerType()) {
            auto pointee = type->getPointeeType();
            auto const is_const = pointee.isLocalConstQualified() ? 1 : 0;

            if (pointee->isRecordType() &&
                !pointee->getAsCXXRecordDecl()->isCompleteDefinition()) {
                print(R"(<pointer name="{}" const="{}" />)",
                      std::string_view(pointee->getAsRecordDecl()->getName()), is_const);
            } else {
                print(R"(<pointer const="{}">)", is_const);
                dump_type(pointee, verbose);
                print(R"(</pointer>)");
            }
        } else if (type->isVoidType()) {
            print(R"(<void />)");
        } else if (type->isRecordType()) {
            auto const* record = type->getAsCXXRecordDecl()->getDefinition();
            auto record_t = record->getTypeForDecl();

            print(R"(<struct name="{}" size="{}" align="{}">)",
                  std::string_view(record->getName()), ctx_.getTypeSize(record_t),
                  ctx_.getTypeAlign(record_t));

            if (verbose) {
                for (auto const* f : record->fields()) {
                    print(R"(<field index="{}" name="{}" size="{}" offset="{}">)",
                          f->getFieldIndex(), std::string_view(f->getName()),
                          ctx_.getTypeSize(f->getType()), ctx_.getFieldOffset(f));
                    dump_type(f->getType(), false);
                    print(R"(</field>)");
                }
            }

            print(R"(</struct>)");
        } else {
            type.dump();
            std::exit(1);
        }
    }

    clang::ASTContext& ctx_;
};

struct ASTConsumer : clang::ASTConsumer {
    virtual void HandleTranslationUnit(clang::ASTContext& ctx) override {
        Visitor(ctx).TraverseAST(ctx);
    }
};

struct FrontendAction : clang::SyntaxOnlyAction {
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&,
                                                                  llvm::StringRef) override {
        return std::make_unique<ASTConsumer>();
    }
};

struct FrontendActionFactory : clang::tooling::FrontendActionFactory {
    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<FrontendAction>();
    }
};

void add_hash(pugi::xml_node node) {
    std::stringstream ss;
    node.print(ss);

    llvm::BLAKE3 hasher;
    hasher.update(ss.str());

    std::string hex;
    for (auto const x : hasher.final()) {
        hex += std::format("{:02x}", x);
    }

    auto attr = node.append_attribute("hash");
    attr.set_value(hex.c_str());
}

}  // namespace

int main(int argc, char** argv) {
    llvm::cl::OptionCategory options("Options");

    auto op = clang::tooling::CommonOptionsParser::create(argc, const_cast<const char**>(argv),
                                                          options);

    if (auto err = op.takeError()) {
        llvm::errs() << err << "\n";
        return 1;
    }

    clang::tooling::ClangTool tool(op->getCompilations(), op->getSourcePathList());

    tool.appendArgumentsAdjuster(clang::tooling::getClangSyntaxOnlyAdjuster());
    tool.appendArgumentsAdjuster(clang::tooling::getClangStripOutputAdjuster());
    tool.appendArgumentsAdjuster(clang::tooling::getClangStripDependencyFileAdjuster());

    g_buffer.reserve(256 * 1024);

    if (auto const status = tool.run(std::make_unique<FrontendActionFactory>().get()); status) {
        return status;
    }

    pugi::xml_document doc;
    doc.load_buffer(g_buffer.data(), g_buffer.size());
    auto top = *doc.root().begin();

    for (auto node : top) {
        add_hash(node);
    }
    add_hash(top);

    doc.save(std::cout, "");

    return 0;
}
