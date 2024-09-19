#include "transform_info.hpp"
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <fmt/format.h>
#include <utils/naming.hpp>
#include <xcml_type.hpp>
#include "utils.hpp"

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

#include <clang/AST/AST.h>
#include <clang/AST/Mangle.h>

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
#if defined(__GNUC__) && defined(__clang__)
#    pragma clang diagnostic pop
#endif

#define COMPARE_TYPE(ty, name_str) \
    else if (type == ctx.ty) {     \
        name = name_str;           \
    }

bool has_annotate(clang::Decl const* decl, std::string_view name) {
    for (auto attr : decl->attrs()) {
        if (auto annotate = clang::dyn_cast<clang::AnnotateAttr>(attr)) {
            auto const& val = annotate->getAnnotation();

            if (std::string_view(val.data(), val.size()) == name) {
                return true;
            }
        }
    }
    return false;
}

struct transform_info::impl {
    explicit impl(clang::ASTContext& c) : ctx(c), prg(xcml::new_xcml_program_node()) {}

    std::string encode_name(clang::QualType const& type) {
        std::unique_ptr<clang::MangleContext> m(ctx.createMangleContext());
        std::string name;
        llvm::raw_string_ostream os(name);

        name.reserve(32);
        m->mangleTypeName(type, os);

        assert(name.size() > 2 && name.substr(0, 2) == "_Z");
        return "__s_" + name.substr(2);
    }

    std::string encode_name(clang::GlobalDecl gd) {
        std::unique_ptr<clang::MangleContext> m(ctx.createMangleContext());
        std::string name;
        llvm::raw_string_ostream os(name);

        name.reserve(32);
        m->mangleName(gd, os);

        assert(name.size() > 2 && name.substr(0, 2) == "_Z");
        return "__s_" + name.substr(2);
    }

    std::string encode_name(clang::Decl const* decl, context const& = context()) {
        PUSH_CONTEXT(decl);

        if (auto record = clang::dyn_cast<clang::RecordDecl>(decl)) {
            return encode_name(ctx.getRecordType(record));
        }
        if (auto ctor = clang::dyn_cast<clang::CXXConstructorDecl>(decl)) {
            return encode_name(clang::GlobalDecl(ctor, clang::Ctor_Complete));
        }
        if (auto fd = clang::dyn_cast<clang::FunctionDecl>(decl)) {
            if (fd->getDeclName().isIdentifier() && fd->getName().startswith("__charm_sycl_")) {
                std::string nns;
                llvm::raw_string_ostream stream(nns);
                fd->printNestedNameSpecifier(stream);

                if (nns == "sycl::runtime::") {
                    return fd->getNameAsString();
                }
            }

            return encode_name(clang::GlobalDecl(fd));
        }

        not_supported(decl, ctx);
    }

    clang::QualType ref_to_ptr(clang::QualType type) {
        if (type->isReferenceType()) {
            return ctx.getPointerType(type.getNonReferenceType());
        }
        return type;
    }

    std::string define_type(transform_info& self, clang::QualType type,
                            context const& = context()) {
        PUSH_CONTEXT(type);

        if (type.isConstQualified() && type->isPointerType()) {
            type.removeLocalConst();
        }

        if (type.isConstQualified() && !type->isPointerType()) {
            PUSH_CONTEXT(type);

            type.removeLocalConst();

            auto bt = xcml::new_basic_type();
            bt->is_const = true;
            bt->type = nm.xbasic_type();
            bt->name = define_type(self, type);
            prg->type_table.push_back(bt);

            return bt->type;
        }

        if (type->isPointerType()) {
            PUSH_CONTEXT(type);

            auto pt = xcml::new_pointer_type();
            pt->type = nm.xptr_type();
            pt->ref = define_type(self, type->getPointeeType());
            prg->type_table.push_back(pt);

            return pt->type;
        }

        if (type->isBuiltinType()) {
            PUSH_CONTEXT(type);

            std::string name;

            if (false) {
            }
            COMPARE_TYPE(VoidTy, "void")
            COMPARE_TYPE(BoolTy, "_Bool")
            COMPARE_TYPE(CharTy, "char")
            COMPARE_TYPE(ShortTy, "short")
            COMPARE_TYPE(IntTy, "int")
            COMPARE_TYPE(LongTy, "long")
            COMPARE_TYPE(LongLongTy, "long_long")
            COMPARE_TYPE(UnsignedCharTy, "unsigned_char")
            COMPARE_TYPE(UnsignedShortTy, "unsigned_short")
            COMPARE_TYPE(UnsignedIntTy, "unsigned")
            COMPARE_TYPE(UnsignedLongTy, "unsigned_long")
            COMPARE_TYPE(UnsignedLongLongTy, "unsigned_long_long")
            COMPARE_TYPE(FloatTy, "float")
            COMPARE_TYPE(DoubleTy, "double")
            COMPARE_TYPE(LongDoubleTy, "long_double")

            if (name.empty()) {
                not_supported(type, ctx);
            }
            return name;
        }

        if (type->isRecordType()) {
            PUSH_CONTEXT(type);

            check_device_copyable(type);
            auto record = type->getAsCXXRecordDecl();
            return define_record(self, type, record);
        }

        if (type->isConstantArrayType() || type->isIncompleteArrayType()) {
            PUSH_CONTEXT(type);

            auto at = clang::dyn_cast<clang::ArrayType>(type);

            auto t = xcml::new_array_type();
            t->type = nm.xbasic_type();
            if (auto cat = clang::dyn_cast<clang::ConstantArrayType>(type)) {
                t->array_size = cat->getSize().getZExtValue();
            } else {
                t->array_size = 0;
            }
            t->element_type = define_type(self, at->getElementType());
            prg->type_table.push_back(t);
            return t->type;
        }

        if (type->isEnumeralType()) {
            PUSH_CONTEXT(type);

            return define_type(
                self,
                clang::dyn_cast<clang::EnumDecl>(type->getAsTagDecl())->getPromotionType());
        }

        if (type->isVectorType()) {
            auto vt = clang::dyn_cast<clang::VectorType>(type);
            auto t = xcml::new_basic_type();

            t->type = nm.xbasic_type();
            t->name = define_type(self, vt->getElementType());
            t->veclen = vt->getNumElements();

            prg->type_table.push_back(t);

            return t->type;
        }

        not_supported(type, ctx);
    }

    std::optional<std::string> find_type(clang::QualType type) const {
        if (auto it = defined_types.find(type); it != defined_types.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void add_type(clang::QualType type, std::string_view name) {
        defined_types.insert_or_assign(type, name);
    }

    std::string define_record(transform_info& self, clang::QualType type,
                              clang::CXXRecordDecl const* record, context const& = context()) {
        PUSH_CONTEXT(record);

        if (auto name = find_type(type)) {
            return *name;
        }

        auto const name = encode_name(type);
        auto const ty = nm.xstruct_type();

        struct_builder st(self, ty, name);

        define_record_fields(st, record, record);

        add_type(type, ty);

        return ty;
    }

    void define_record_fields(struct_builder& st, clang::CXXRecordDecl const* /*origin*/,
                              clang::CXXRecordDecl const* record, context const& = context()) {
        PUSH_CONTEXT(record);

        auto const l = layout(ctx, ctx.getRecordType(record));

        if (l.empty()) {
            st.add("__s_dummy", ctx.CharTy);
        } else {
            for (auto it = l.begin(); it != l.end(); ++it) {
                auto type = it->decl()->getType();
                type.removeLocalFastQualifiers();
                st.add(l.get_field_name(it->decl()), type);
            }
        }
    }

    bool is_device_copyable(clang::CXXRecordDecl const* decl, context const& = context()) {
        PUSH_CONTEXT(decl);

        return is_device_copyable(ctx.getRecordType(decl));
    }

    bool is_device_copyable(clang::QualType type, context const& = context()) {
        PUSH_CONTEXT(type);

        return is_device_copyable(type, type->getAsCXXRecordDecl());
    }

    bool is_device_copyable(clang::QualType type, clang::CXXRecordDecl const* decl,
                            context const& = context()) {
        PUSH_CONTEXT(type);

        assert(type->isRecordType());
        assert(!type->isPointerType());

        if (type.isLocalConstQualified()) {
            type.removeLocalConst();
        }
        if (type.isVolatileQualified()) {
            type.removeLocalVolatile();
        }

        assert(type.getQualifiers().empty());

        if (type.isTriviallyCopyableType(ctx)) {
            return true;
        }

        {
            PUSH_CONTEXT(decl);
            return has_annotate(decl, "charm_sycl_device_copyable");
        }
    }

    void check_device_copyable(clang::CXXRecordDecl const* decl, context const& = context()) {
        if (!is_device_copyable(decl)) {
            not_supported(decl, ctx);
        }
    }

    void check_device_copyable(clang::QualType type, context const& = context()) {
        if (!is_device_copyable(type)) {
            not_supported(type, ctx);
        }
    }

    void check_device_copyable(clang::QualType type, clang::CXXRecordDecl const* decl,
                               context const& = context()) {
        if (!is_device_copyable(type, decl)) {
            not_supported(type, ctx);
        }
    }

    clang::ASTContext& ctx;
    xcml::xcml_program_node_ptr prg;
    utils::naming_utils nm;
    std::map<clang::QualType, std::string> defined_types;
    std::unordered_map<int64_t, std::string> sym_map;
    std::unordered_map<std::string, transform_info::func_desc> defined_funcs;
    std::unordered_map<clang::ForStmt const*, xcml::expr_ptr> for_cond_map;
    std::optional<capture_map_t> current_captures;
    llvm::DenseSet<clang::CXXRecordDecl const*> kernel_records;
};

transform_info::transform_info(clang::ASTContext& ctx) : pimpl_(std::make_unique<impl>(ctx)) {}

transform_info::~transform_info() = default;

std::string const& transform_info::define_type(clang::QualType const& type, context const&) {
    PUSH_CONTEXT(type);

    auto const ctype = pimpl_->ctx.getCanonicalType(pimpl_->ref_to_ptr(type));

    {
        PUSH_CONTEXT(ctype);
        auto& m = pimpl_->defined_types;

        auto it = m.find(ctype);
        if (it == m.end()) {
            it = m.insert(std::make_pair(ctype, pimpl_->define_type(*this, ctype))).first;
        }
        return it->second;
    }
}

std::string const& transform_info::rename_sym(clang::NamedDecl const* decl, context const&) {
    PUSH_CONTEXT(decl);

    auto& m = pimpl_->sym_map;

    if (auto it = m.find(decl->getID()); it != m.end()) {
        return it->second;
    }

    auto const renamed = pimpl_->nm.user_var(decl->getName());

    return m.insert_or_assign(decl->getID(), renamed).first->second;
}

std::string transform_info::encode_name(clang::Decl const* decl, context const&) {
    return pimpl_->encode_name(decl);
}

void transform_info::check_device_copyable(clang::CXXRecordDecl const* decl) {
    pimpl_->check_device_copyable(decl);
}

void transform_info::add_type(clang::QualType const& type, std::string_view name) {
    pimpl_->add_type(type, name);
}

void transform_info::add_func(std::string_view name, xcml::function_type_ptr const& ft,
                              xcml::function_decl_ptr const& decl,
                              xcml::func_addr_ptr const& addr) {
    func_desc desc;
    desc.type = ft;
    desc.decl = decl;
    desc.addr = addr;
    pimpl_->defined_funcs.insert_or_assign(std::string(name), desc);
}

std::optional<transform_info::func_desc> transform_info::find_func(
    std::string_view name) const {
    auto const& m = pimpl_->defined_funcs;

    if (auto const it = m.find(std::string(name)); it != m.end()) {
        return it->second;
    }

    return std::nullopt;
}

bool transform_info::is_func_defined(std::string_view name) const {
    auto const& m = pimpl_->defined_funcs;
    auto const it = m.find(std::string(name));
    return it != m.end();
}

xcml::xcml_program_node_ptr transform_info::prg() {
    return pimpl_->prg;
}

utils::naming_utils& transform_info::nm() {
    return pimpl_->nm;
}

clang::ASTContext& transform_info::ctx() {
    return pimpl_->ctx;
}

clang::ASTContext const& transform_info::ctx() const {
    return pimpl_->ctx;
}

void transform_info::add_for_condition(clang::ForStmt const* stmt, xcml::expr_ptr cond) {
    pimpl_->for_cond_map.insert_or_assign(stmt, cond);
}

xcml::expr_ptr transform_info::lookup_for_condition(clang::ForStmt const* stmt) {
    auto const it = pimpl_->for_cond_map.find(stmt);
    if (it == pimpl_->for_cond_map.end()) {
        return nullptr;
    }
    return it->second;
}

scoped_set<std::optional<capture_map_t>> transform_info::scoped_set_captures(
    clang::CXXRecordDecl const* decl) {
    capture_map_t captures;
    clang::FieldDecl* this_capture;

    decl->getCaptureFields(captures, this_capture);

    return scoped_set(pimpl_->current_captures, std::make_optional(captures));
}

std::optional<capture_map_t> const& transform_info::current_captures() const {
    return pimpl_->current_captures;
}

void transform_info::add_kernel(clang::CXXRecordDecl const* record) {
    pimpl_->kernel_records.insert(record->getCanonicalDecl());
}

bool transform_info::is_kernel(clang::CXXRecordDecl const* record) {
    return pimpl_->kernel_records.contains(record->getCanonicalDecl());
}
