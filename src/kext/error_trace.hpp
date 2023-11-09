#pragma once

#include <string_view>

namespace clang {
class ASTContext;
class Decl;
class QualType;
class Stmt;
}  // namespace clang

struct source_location {
    explicit source_location(char const* file = __builtin_FILE(),
                             char const* func = __builtin_FUNCTION(),
                             int line = __builtin_LINE())
        : file_(file), func_(func), line_(line) {}

    [[nodiscard]] char const* file() const {
        return file_;
    }

    [[nodiscard]] char const* func() const {
        return func_;
    }

    [[nodiscard]] int line() const {
        return line_;
    }

private:
    char const* file_;
    char const* func_;
    int line_;
};

void push_error_ctx(source_location const& loc);
void push_error_ctx(clang::Stmt const* stmt, source_location const& loc);
void push_error_ctx(clang::Decl const* decl, source_location const& loc);
void push_error_ctx(clang::QualType const& type, source_location const& loc);
void pop_error_ctx();

struct context {
    explicit context(source_location const& loc = source_location()) {
        push_error_ctx(loc);
    }

    explicit context(clang::Stmt const* stmt, source_location const& loc = source_location()) {
        push_error_ctx(stmt, loc);
    }

    explicit context(clang::Decl const* decl, source_location const& loc = source_location()) {
        push_error_ctx(decl, loc);
    }

    explicit context(clang::QualType const& type,
                     source_location const& loc = source_location()) {
        push_error_ctx(type, loc);
    }

    ~context() {
        pop_error_ctx();
    }
};

[[noreturn]] void not_supported(clang::ASTContext const& ctx, std::string_view msg,
                                source_location const& loc = source_location());

[[noreturn]] void not_supported(clang::QualType const& type, clang::ASTContext const& ctx,
                                std::string_view msg = {},
                                source_location const& loc = source_location());

[[noreturn]] void not_supported(clang::Decl const* decl, clang::ASTContext const& ctx,
                                std::string_view msg = {},
                                source_location const& loc = source_location());

[[noreturn]] void not_supported(clang::Stmt const* stmt, clang::ASTContext const& ctx,
                                std::string_view msg = {},
                                source_location const& loc = source_location());

#define PUSH_CONTEXT(...) auto const dummy__ = ::context(__VA_ARGS__)
#define PUSH_CONTEXT2(...) auto const dummy__2 = ::context(__VA_ARGS__)
