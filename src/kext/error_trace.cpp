#include "error_trace.hpp"
#include <cstdio>
#include <iterator>
#include <string>
#include <utility>
#include <variant>
#include <clang/AST/ASTDumper.h>
#include <fmt/compile.h>
#include <fmt/format.h>

using variant_t =
    std::variant<std::monostate, clang::Stmt const*, clang::Decl const*, clang::QualType>;

static clang::SourceLocation get_source_loc(variant_t const& obj) {
    struct {
        clang::SourceLocation operator()(std::monostate) const {
            return {};
        }
        clang::SourceLocation operator()(clang::Stmt const* stmt) const {
            return stmt->getBeginLoc();
        }
        clang::SourceLocation operator()(clang::Decl const* decl) const {
            return decl->getBeginLoc();
        }
        clang::SourceLocation operator()(clang::QualType const&) const {
            return {};
        }
    } f;
    return std::visit(f, obj);
}

static char const* get_obj_name(variant_t const& obj) {
    struct {
        char const* operator()(std::monostate) const {
            return "";
        }
        char const* operator()(clang::Stmt const* stmt) const {
            return stmt->getStmtClassName();
        }
        char const* operator()(clang::Decl const* decl) const {
            return decl->getDeclKindName();
        }
        char const* operator()(clang::QualType const&) const {
            return "Type";
        }
    } f;
    return std::visit(f, obj);
}

template <class T>
void dump_clang_ast(std::string& out, clang::ASTContext const& ctx, T obj) {
    auto stream = llvm::raw_string_ostream(out);
    clang::ASTDumper dumper(stream, ctx, false);
    dumper.Visit(obj);
    stream.flush();
}

struct error_context {
    explicit error_context(source_location const& loc);

    explicit error_context(clang::Stmt const* stmt, source_location const& loc);

    explicit error_context(clang::Decl const* decl, source_location const& loc);

    explicit error_context(clang::QualType const& type, source_location const& loc);

    void set_prev(std::unique_ptr<error_context>&& prev) noexcept;

    std::unique_ptr<error_context> get_prev();

    void dump(std::string& out, clang::ASTContext const& ctx) const;

private:
    std::unique_ptr<error_context> prev_;
    std::variant<std::monostate, clang::Stmt const*, clang::Decl const*, clang::QualType> obj_;
    source_location loc_;
};

/* Assume single thread */
static std::unique_ptr<error_context> g_ctx;

error_context::error_context(source_location const& loc) : prev_(), obj_(), loc_(loc) {}

error_context::error_context(clang::Stmt const* stmt, source_location const& loc)
    : prev_(), obj_(stmt), loc_(loc) {}

error_context::error_context(clang::Decl const* decl, source_location const& loc)
    : prev_(), obj_(decl), loc_(loc) {}

error_context::error_context(clang::QualType const& type, source_location const& loc)
    : prev_(), obj_(type), loc_(loc) {}

void error_context::set_prev(std::unique_ptr<error_context>&& prev) noexcept {
    prev_ = std::move(prev);
}

std::unique_ptr<error_context> error_context::get_prev() {
    return std::exchange(prev_, {});
}

void error_context::dump(std::string& out, clang::ASTContext const& ctx) const {
    auto it = std::back_inserter(out);
    auto stream = llvm::raw_string_ostream(out);

    for (error_context const* ec = this; ec; ec = ec->prev_.get()) {
        auto const& loc = ec->loc_;

        fmt::format_to(it, FMT_COMPILE("            from {}:{}: {}()\n"), loc.file(),
                       loc.line(), loc.func());

        if (!std::holds_alternative<std::monostate>(ec->obj_)) {
            auto const sloc = get_source_loc(ec->obj_);
            if (sloc.isValid()) {
                auto const* name = get_obj_name(ec->obj_);

                if (name == nullptr || !name[0]) {
                    fmt::format_to(it, FMT_COMPILE("while processing at "));
                } else {
                    fmt::format_to(it, FMT_COMPILE("while processing {} at "), name);
                }

                sloc.print(stream, ctx.getSourceManager());
                stream.flush();

                fmt::format_to(it, "\n");
            }
        }
    }
}

static void push_error_ctx(std::unique_ptr<error_context>&& ctx) {
    ctx->set_prev(std::exchange(g_ctx, {}));
    g_ctx = std::move(ctx);
}

void push_error_ctx(source_location const& loc) {
    push_error_ctx(std::make_unique<error_context>(loc));
}

void push_error_ctx(clang::Stmt const* stmt, source_location const& loc) {
    push_error_ctx(std::make_unique<error_context>(stmt, loc));
}

void push_error_ctx(clang::Decl const* decl, source_location const& loc) {
    push_error_ctx(std::make_unique<error_context>(decl, loc));
}

void push_error_ctx(clang::QualType const& type, source_location const& loc) {
    push_error_ctx(std::make_unique<error_context>(type, loc));
}

void pop_error_ctx() {
    g_ctx = g_ctx->get_prev();
}

static error_context const* get_current_error_ctx() {
    return g_ctx.get();
}

template <class T>
[[noreturn]] static void not_supported(clang::ASTContext const& ctx, std::string_view msg,
                                       char const* obj_kind, T const& obj,
                                       source_location const& loc) {
    std::string errmsg;
    auto it = std::back_inserter(errmsg);

    if constexpr (!std::is_null_pointer_v<T>) {
        fmt::format_to(it, FMT_COMPILE("Not supported {}:"), obj_kind);
    }
    if (!msg.empty()) {
        fmt::format_to(it, FMT_COMPILE(" {}\n\n"), msg);
    } else {
        fmt::format_to(it, FMT_COMPILE("\n\n"), msg);
    }

    if constexpr (!std::is_null_pointer_v<T>) {
        dump_clang_ast(errmsg, ctx, obj);
    }
    fmt::format_to(it, "\n     called from {}:{}: {}()\n", loc.file(), loc.line(), loc.func());

    get_current_error_ctx()->dump(errmsg, ctx);

    fmt::print(stderr, FMT_COMPILE("\n----------\nFatal Error:\n{}\n----------\n"), errmsg);

    std::exit(1);
}

void not_supported(clang::ASTContext const& ctx, std::string_view msg,
                   source_location const& loc) {
    not_supported(ctx, msg, nullptr, nullptr, loc);
}

void not_supported(clang::QualType const& type, clang::ASTContext const& ctx,
                   std::string_view msg, source_location const& loc) {
    not_supported(ctx, msg, "QualType", type, loc);
}

void not_supported(clang::Decl const* decl, clang::ASTContext const& ctx, std::string_view msg,
                   source_location const& loc) {
    not_supported(ctx, msg, "Declaration", decl, loc);
}

void not_supported(clang::Stmt const* stmt, clang::ASTContext const& ctx, std::string_view msg,
                   source_location const& loc) {
    if (clang::isa<clang::Expr>(stmt)) {
        not_supported(ctx, msg, "Expression", stmt, loc);
    }
    not_supported(ctx, msg, "Statement", stmt, loc);
}
