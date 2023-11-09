#include "rewrite.hpp"
#include <map>
#include <string>
#include <unordered_set>
#include <clang/Basic/SourceManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <fmt/format.h>
#include <llvm/Support/raw_ostream.h>

namespace {

std::string print_expr(clang::ASTContext& ctx, clang::Expr const* expr) {
    std::string buffer;
    llvm::raw_string_ostream os(buffer);
    clang::PrintingPolicy pp(ctx.getLangOpts());

    expr->printPretty(os, nullptr, pp, 0u, "", &ctx);

    return buffer;
}

struct transformer_impl : transformer {
    static clang::CXXRecordDecl const* as_functor(clang::Expr const* expr) {
        if (auto const* dr = clang::dyn_cast<clang::DeclRefExpr>(expr)) {
            return dr->getDecl()->getType()->getAsCXXRecordDecl();
        }
        if (auto const* ctor = clang::dyn_cast<clang::CXXConstructExpr>(expr)) {
            return ctor->getType()->getAsCXXRecordDecl();
        }
        return nullptr;
    }

    static clang::LambdaExpr const* as_lambda(clang::Expr const* expr) {
        return clang::dyn_cast<clang::LambdaExpr>(expr);
    }

    void rewrite(clang::Rewriter& r, clang::ASTContext& ctx, llvm::StringRef name,
                 clang::CXXMemberCallExpr* call, clang::Expr* range, clang::Expr* fn) override {
        if (auto lambda = as_lambda(fn)) {
            auto const loc = call->getEndLoc();
            if (lambdas_.find(loc) != lambdas_.end()) {
                return;
            }

            auto cgh = call->getImplicitObjectArgument();
            auto cgh_str = print_expr(ctx, cgh);

            entry e;
            e.cgh = cgh_str;
            e.lambda = lambda;

            for (auto const* init : lambda->capture_inits()) {
                e.captures.push_back(print_expr(ctx, init));
            }

            lambdas_.insert_or_assign(loc, e);

            return;
        }

        if (auto const* record = as_functor(fn)) {
            if (records_.count(record)) {
                return;
            }
            records_.insert(record);
            return;
        }

        fmt::print(stderr, "{}:{}:\nNot supported {}:\n", __FILE__, __LINE__,
                   fn->getStmtClassName());
        fn->dumpColor();
        std::exit(1);
    }

    void finalize(clang::Rewriter& r) override {
        for (auto const* record : records_) {
            auto const loc = record->getEndLoc();

            std::string h, klass;

            if (auto const* spec =
                    clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(record)) {
                clang::PrintingPolicy pp(r.getLangOpts());
                llvm::raw_string_ostream stream(klass);

                klass = fmt::format("{}<", sv(spec->getName()));

                auto const& args = spec->getTemplateInstantiationArgs();
                for (unsigned i = 0; i < args.size(); i++) {
                    if (i > 0) {
                        klass += ',';
                    }

                    args[i].print(pp, stream, false);
                }

                klass += '>';
            } else {
                klass = record->getNameAsString();
            }

            h += "\n/* BEGIN: chsy-host-rewrite */\n";
            h += "public:\n";

            h += "template <class Handler> static void __do_binds(Handler& h";
            h += fmt::format(", {} const& obj", klass);
            h += ") {\n";
            h += "h.__bind(";

            bool comma = false;
            for (auto const* field : record->fields()) {
                auto const& name = field->getName();

                if (comma) {
                    h += ", ";
                }
                h += fmt::format("obj.{}", sv(name));

                comma = true;
            }

            h += ");\n}\n";

            h += "\n/* END: chsy-host-rewrite */\n";

            r.InsertTextBefore(loc, h);
        }

        for (auto const& pair : lambdas_) {
            auto const& loc = pair.first;
            auto const& ent = pair.second;

            r.InsertTextAfterToken(loc, ";\n/* BEGIN: chsy-host-rewrite */\n");
            r.InsertTextAfterToken(loc, fmt::format("({}).__bind(", ent.cgh));

            bool comma = false;
            for (auto const& var : ent.captures) {
                if (comma) {
                    r.InsertTextAfterToken(loc, ", ");
                }
                r.InsertTextAfterToken(loc, var);

                comma = true;
            }

            r.InsertTextAfterToken(loc, ")\n");
            r.InsertTextAfterToken(loc, "\n/* END: chsy-host-rewrite */\n");
        }
    }

private:
    std::string_view sv(llvm::StringRef const& str) {
        return std::string_view(str.data(), str.size());
    }

    struct entry {
        std::string cgh;
        std::vector<std::string> captures;
        clang::LambdaExpr const* lambda;
    };
    std::map<clang::SourceLocation, entry> lambdas_;
    std::unordered_set<clang::CXXRecordDecl const*> records_;
};

}  // namespace

std::unique_ptr<transformer> make_transformer() {
    return std::make_unique<transformer_impl>();
}
