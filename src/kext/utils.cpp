#include "utils.hpp"
#include <vector>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <xcml_type.hpp>
#include "error_trace.hpp"
#include "transform_info.hpp"

static std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> captured_vars(
    transform_info&, clang::LambdaExpr const* lambda, context const& = context()) {
    PUSH_CONTEXT(lambda);

#if LLVM_VERSION_MAJOR <= 15
    llvm::DenseMap<const clang::VarDecl*, clang::FieldDecl*> captures;
#else
    llvm::DenseMap<const clang::ValueDecl*, clang::FieldDecl*> captures;
#endif
    clang::FieldDecl* this_capture;
    std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> res;

    lambda->getLambdaClass()->getCaptureFields(captures, this_capture);

    /* Order must be same as lambda->captures() */
    for (auto const c : lambda->captures()) {
        auto const* vd = c.getCapturedVar();
        auto const* fd = captures[vd];
        res.emplace_back(vd, fd);
    }

    return res;
}

static std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> captured_vars(
    transform_info& info, clang::CXXRecordDecl const* record, context const& = context()) {
    PUSH_CONTEXT(record);

    std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> res;

    for (auto const* field : record->fields()) {
        if (field->isMutable()) {
            not_supported(record, info.ctx(), "mutable is not allowed");
        }

        res.emplace_back(field, field);
    }

    return res;
}

std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> captured_vars(
    transform_info& info, clang::Expr const* fn, context const&) {
    PUSH_CONTEXT(fn);

    if (auto const* lambda = clang::dyn_cast<clang::LambdaExpr>(fn)) {
        return captured_vars(info, lambda);
    }

    if (auto const* dr = clang::dyn_cast<clang::DeclRefExpr>(fn)) {
        return captured_vars(info, dr->getDecl()->getType()->getAsCXXRecordDecl());
    }

    if (auto const* ctor = clang::dyn_cast<clang::CXXConstructExpr>(fn)) {
        return captured_vars(info, ctor->getType()->getAsCXXRecordDecl());
    }

    not_supported(fn, info.ctx());
}

bool is_const_lvref(clang::QualType const& type) {
    return type->isLValueReferenceType() && type->getPointeeType().isLocalConstQualified();
}

void remove_const_lvref(clang::QualType& type) {
    if (is_const_lvref(type)) {
        type = type->getPointeeType();
        type.removeLocalConst();
    }
}

bool is_empty(xcml::compound_stmt_ptr c) {
    return c->declarations.empty() && c->symbols.empty() && c->body.empty();
}
