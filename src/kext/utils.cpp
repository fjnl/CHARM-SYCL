#include "utils.hpp"
#include <vector>
#include <fmt/format.h>
#include <xcml_type.hpp>
#include "error_trace.hpp"
#include "transform_info.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#    pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-parameter"
#    pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/RecordLayout.h>

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

bool is_accessor(clang::QualType type, accessor_type& acc) {
    acc = accessor_type::NONE;

    if (auto const* record = type->getAsCXXRecordDecl()) {
        while (record) {
            auto const name = record->getQualifiedNameAsString();

            if (name.size() == 14 && name == "sycl::accessor") {
                acc = accessor_type::NORMAL;
            } else if (name.size() == 20 && name == "sycl::local_accessor") {
                acc = accessor_type::LOCAL;
            } else if (name.size() == 29 && name == "sycl::detail::device_accessor") {
                acc = accessor_type::DEVICE;
            }

            if (record->bases().empty()) {
                record = nullptr;
            } else {
                record = record->bases_begin()->getType()->getAsCXXRecordDecl();
            }
        }
    }

    return (acc != accessor_type::NONE);
}

clang::CXXMethodDecl const* get_call_operator(transform_info& info,
                                              clang::CXXRecordDecl const* record) {
    if (auto const* cts = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(record)) {
        record = cts;
    }

    for (auto const& method : record->methods()) {
        // TODO: consider overloading
        if (method->getOverloadedOperator() == clang::OO_Call) {
            return method;
        }
    }

    not_supported(record, info.ctx(), "unable to find the call operator");
}

clang::CXXMethodDecl const* get_call_operator(transform_info& info, clang::Expr const* fn) {
    if (auto const* call = clang::dyn_cast<clang::CXXOperatorCallExpr>(fn)) {
        return clang::dyn_cast<clang::CXXMethodDecl>(call->getCalleeDecl());
    }

    if (auto const* lambda = clang::dyn_cast<clang::LambdaExpr>(fn)) {
        return lambda->getCallOperator();
    }

    if (auto const* dr = clang::dyn_cast<clang::DeclRefExpr>(fn)) {
        return get_call_operator(info,
                                 remove_cvref(dr->getDecl()->getType())->getAsCXXRecordDecl());
    }

    if (auto const* ctor = clang::dyn_cast<clang::CXXConstructExpr>(fn)) {
        return get_call_operator(info, ctor->getType()->getAsCXXRecordDecl());
    }

    not_supported(fn, info.ctx(), "unable to find the call operator");
}

static std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>>
captured_vars_lambda(transform_info&, clang::CXXRecordDecl const* lambda_class,
                     context const& = context()) {
    PUSH_CONTEXT(lambda_class);

#if LLVM_VERSION_MAJOR <= 15
    llvm::DenseMap<const clang::VarDecl*, clang::FieldDecl*> captures;
#else
    llvm::DenseMap<const clang::ValueDecl*, clang::FieldDecl*> captures;
#endif
    clang::FieldDecl* this_capture;
    std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> res;

    lambda_class->getCaptureFields(captures, this_capture);

    for (auto const c : lambda_class->captures()) {
        auto const* vd = c.getCapturedVar();
        auto const* fd = captures[vd];
        res.emplace_back(vd, fd);
    }

    return res;
}

static std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> captured_vars(
    transform_info& info, clang::LambdaExpr const* lambda, context const& = context()) {
    PUSH_CONTEXT(lambda);

    return captured_vars_lambda(info, lambda->getLambdaClass());
}

static std::vector<std::pair<clang::ValueDecl const*, clang::FieldDecl const*>> captured_vars(
    transform_info& info, clang::CXXRecordDecl const* record, context const& = context()) {
    PUSH_CONTEXT(record);

    if (record->isLambda()) {
        return captured_vars_lambda(info, record);
    }

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
        return captured_vars(info,
                             remove_cvref(dr->getDecl()->getType())->getAsCXXRecordDecl());
    }

    if (auto const* ctor = clang::dyn_cast<clang::CXXConstructExpr>(fn)) {
        return captured_vars(info, ctor->getType()->getAsCXXRecordDecl());
    }

    not_supported(fn, info.ctx());
}

bool is_const_lvref(clang::QualType const& type) {
    return type->isLValueReferenceType() && type->getPointeeType().isLocalConstQualified();
}

clang::QualType remove_const_lvref(clang::QualType type) {
    if (type->isLValueReferenceType()) {
        return remove_cvref(type);
    }
    return type;
}

clang::QualType remove_cvref(clang::QualType type) {
    type = type.getNonReferenceType();
    type.removeLocalConst();
    type.removeLocalVolatile();
    return type;
}

bool is_empty(xcml::compound_stmt_ptr c) {
    return c->declarations.empty() && c->symbols.empty() && c->body.empty();
}

namespace {

void collect_bases(clang::ASTContext& ctx, clang::CXXRecordDecl const* record,
                   std::vector<clang::CXXRecordDecl const*>& result) {
    for (auto base : record->bases()) {
        auto const* base_decl = base.getType()->getAsCXXRecordDecl();
        collect_bases(ctx, base_decl, result);
    }

    result.push_back(record);
}

}  // namespace

struct layout::impl {
    clang::ASTContext* ctx;
    clang::QualType type;
    clang::CXXRecordDecl const* decl;
    std::vector<field> fields;

    void init(layout* self, bool recursive) {
        std::vector<clang::CXXRecordDecl const*> bases;
        collect_bases(*ctx, decl, bases);

        fields.clear();
        for (auto const* base : bases) {
            for (auto const* field : base->fields()) {
                if (recursive) {
                    std::vector<clang::FieldDecl const*> history;
                    add_field(self, field, 0, &history, recursive);
                } else {
                    add_field(self, field, 0);
                }
            }
        }

        std::stable_sort(fields.begin(), fields.end(), [](auto const& lhs, auto const& rhs) {
            return lhs.offset_of() < rhs.offset_of();
        });
    }

    void add_field(layout* self, clang::FieldDecl const* field, size_t base_off,
                   std::vector<clang::FieldDecl const*>* history = nullptr,
                   bool recursive = false) {
        auto const* record = field->getParent();
        auto const& layout = ctx->getASTRecordLayout(record);
        auto const field_off = layout.getFieldOffset(field->getFieldIndex()) / 8;
        auto const field_type = field->getType();
        auto const* field_record = field_type->getAsCXXRecordDecl();

        if (history) {
            history->push_back(field);
            fields.emplace_back(*self, *history, base_off + field_off);
        } else {
            fields.emplace_back(*self, field, base_off + field_off);
        }

        if (recursive && field_record) {
            add_record(self, field_record, base_off + field_off, history, recursive);
        }

        if (history) {
            history->pop_back();
        }
    }

    void add_record(layout* self, clang::CXXRecordDecl const* record, size_t base_off,
                    std::vector<clang::FieldDecl const*>* history = nullptr,
                    bool recursive = false) {
        for (auto const* f : record->fields()) {
            add_field(self, f, base_off, history, recursive);
        }
    }

    size_t size_of() const {
        return ctx->getASTRecordLayout(decl).getSize().getQuantity();
    }

    size_t align_of() const {
        return ctx->getASTRecordLayout(decl).getAlignment().getQuantity();
    }
};

layout::layout(clang::ASTContext& ctx, clang::QualType const& type, bool recursive)
    : pimpl_(new impl) {
    pimpl_->ctx = &ctx;
    pimpl_->type = ctx.getCanonicalType(type);
    pimpl_->decl = pimpl_->type->getAsCXXRecordDecl();
    pimpl_->init(this, recursive);
}

layout::~layout() {}

void layout::dump() const {
    fmt::print(stderr, "record `{}`, sizeof(T)={}, alignof(T)={}\n",
               pimpl_->decl->getNameAsString(), size_of(), align_of());
    for (auto const& f : pimpl_->fields) {
        std::string p_str;
        bool is_record = f.decl()->getType()->isRecordType();

        for (auto const* p : f.path()) {
            if (!p_str.empty()) {
                p_str += '/';
            }
            p_str += get_field_name(p);
        }

        fmt::print(stderr, "  field record={}, nest={}, offset={}, size={}, path=[{}]\n",
                   is_record, f.path().size(), f.offset_of(), f.size_of(), p_str);
    }
}

size_t layout::size_of() const {
    return pimpl_->size_of();
}

size_t layout::align_of() const {
    return pimpl_->align_of();
}

clang::QualType const& layout::type() const {
    return pimpl_->type;
}

layout::field const* layout::begin() const {
    if (empty()) {
        return nullptr;
    }
    return &pimpl_->fields.front();
}

layout::field const* layout::end() const {
    if (empty()) {
        return nullptr;
    }
    return &pimpl_->fields.back() + 1;
}

bool layout::empty() const {
    return pimpl_->fields.empty();
}

std::string layout::to_string() const {
    for (auto const& f : pimpl_->fields) {
        accessor_type acc_type;
        if (is_accessor(f.decl()->getType(), acc_type)) {
        }
    }
    return "";

    // fmt::format()
}

std::string layout::get_field_name(clang::FieldDecl const* field) const {
    if (field->getName().empty()) {
        return fmt::format("anon{}", field->getFieldIndex());
    } else {
        return field->getNameAsString();
    }
}

layout::field::field(layout& l, clang::FieldDecl const* decl, size_t offset)
    : field(l, std::vector({decl}), offset) {}

layout::field::field(layout& l, std::vector<clang::FieldDecl const*> const& path, size_t offset)
    : l_(&l), path_(path), offset_(offset) {}
//   pad_(0),
//   final_(decl()->getFieldIndex() + 1 == std::distance(decl()->getParent()->field_begin(),
//                                                       decl()->getParent()->field_end())) {}

size_t layout::field::offset_of() const {
    return offset_;
}

size_t layout::field::size_of() const {
    return l_->pimpl_->ctx->getTypeSize(decl()->getType()) / 8;
}

clang::FieldDecl const* layout::field::decl() const {
    return path_.back();
}

std::vector<clang::FieldDecl const*> const& layout::field::path() const {
    return path_;
}

static clang::QualType real_type(transform_info& info, clang::Expr const* expr) {
    if (auto const* ce = clang::dyn_cast<clang::CastExpr>(expr)) {
        auto const kind = ce->getCastKind();

        if (kind == clang::CK_LValueToRValue || kind == clang::CK_NoOp) {
            return real_type(info, ce->getSubExpr());
        }
        if (kind == clang::CK_UncheckedDerivedToBase) {
            return info.ctx().getLValueReferenceType(ce->getType());
        }
    }

    if (auto const* call = clang::dyn_cast<clang::CallExpr>(expr)) {
        return call->getCallReturnType(info.ctx());
    }

    if (auto const* dr = clang::dyn_cast<clang::DeclRefExpr>(expr)) {
        if (dr->isNonOdrUse()) {
            return dr->getType();
        }

        if (dr->refersToEnclosingVariableOrCapture()) {
            auto& captures = info.current_captures();

            if (!captures) {
                not_supported(expr, info.ctx(), "cannot find the original value");
            }

            auto it = captures->find(clang::dyn_cast<capture_key_t>(dr->getDecl()));

            if (it == captures->end()) {
                not_supported(expr, info.ctx(), "cannot find the original value");
            }

            return it->second->getType();
        }

        return dr->getDecl()->getType();
    }

    if (auto const* ase = clang::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
        return info.ctx().getLValueReferenceType(ase->getType());
    }

    return clang::QualType();
}

bool point_to_refenrece(transform_info& info, clang::Expr const* expr) {
    auto type = real_type(info, expr);

    return !type.isNull() && type->isReferenceType();
}

bool point_to_pointer(transform_info& info, clang::Expr const* expr) {
    auto type = real_type(info, expr);

    return !type.isNull() && type->isPointerType();
}

bool point_to_ref_or_ptr(transform_info& info, clang::Expr const* expr) {
    auto type = real_type(info, expr);

    return !type.isNull() && (type->isPointerType() || type->isReferenceType());
}
