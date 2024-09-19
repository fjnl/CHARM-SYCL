#include "transform.hpp"
#include <cstdlib>
#include <clang/AST/AST.h>
#include <clang/AST/StmtVisitor.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <utils/naming.hpp>
#include <xcml.hpp>
#include <charm/sycl/fnv1a.hpp>
#include "error_trace.hpp"
#include "transform_info.hpp"
#include "utils.hpp"
#include "visitor.hpp"

// #define NO_DUMP

// For debugging
#if !defined(NDEBUG) && !defined(NO_DUMP)
#    define DUMP_COLOR(obj)                                                                    \
        ({                                                                                     \
            auto const* obj__ = (obj);                                                         \
            fmt::print(stderr, ">------ {}:{}  {} in {}\n", __FILE__, __LINE__,                \
                       (obj)->getBeginLoc().printToString(ast_.getSourceManager()), __func__); \
            obj__->dumpColor();                                                                \
            fmt::print(stderr, "<------ {}:{}\n\n", __FILE__, __LINE__);                       \
        })

#    define DUMP(obj)                                                                          \
        ({                                                                                     \
            auto const* obj__ = (obj);                                                         \
            fmt::print(stderr, ">------ {}:{}  {} in {}\n", __FILE__, __LINE__,                \
                       (obj)->getBeginLoc().printToString(ast_.getSourceManager()), __func__); \
            obj__->dump();                                                                     \
            fmt::print(stderr, "<------ {}:{}\n\n", __FILE__, __LINE__);                       \
        })
#else
#    define DUMP_COLOR(obj)
#    define DUMP(obj)
#endif

namespace {

using namespace xcml::utils;

[[maybe_unused]] static constexpr bool STATIC = true, EXTERN = false;
[[maybe_unused]] static constexpr bool EXTERN_C = true, NO_EXTERN_C = false;

static constexpr char const* const DESC_PRE = R"(
#include <string_view>
#include <charm/sycl/fwd.hpp>
namespace {
namespace __cham_sycl_kernel_descs {

struct dummy {};

template <class T, size_t Offset>
struct acc_desc {
    constexpr auto offset() const {
        return Offset;
    }

    T* interpret(std::byte* ptr) const {
        return reinterpret_cast<T*>(ptr + Offset);
    }
};

struct list_0 {
    constexpr size_t size() const {
        return 0;
    }

    constexpr size_t empty() const {
        return size() == 0;
    }
};

template <auto Value>
struct list_1 {
    constexpr auto head() const {
        return Value;
    }

    constexpr auto tail() const {
        return list_0{};
    }

    constexpr size_t size() const {
        return 1;
    }

    constexpr size_t empty() const {
        return size() == 0;
    }
};

template <auto Head, auto... Tail>
struct list_n {
    constexpr auto head() const {
        return Head;
    }

    constexpr auto tail() const {
        if constexpr (sizeof...(Tail) == 1) {
            return list_1<Tail...>{};
        } else {
            return list_n<Tail...>{};
        }
    }

    constexpr size_t size() const {
        return 1 + sizeof...(Tail);
    }

    constexpr size_t empty() const {
        return size() == 0;
    }
};

template <auto... Values>
constexpr auto list() {
    if constexpr (sizeof...(Values) == 0) {
        return list_0{};
    } else if constexpr (sizeof...(Values) == 1) {
        return list_1<Values...>{};
    } else {
        return list_n<Values...>{};
    }
}

template <size_t N>
struct str_wrapper {
    constexpr str_wrapper(char const* s) {
        for (size_t i = 0; i < N; i++) {
            data[i] = s[i];
        }
    }

    constexpr auto sv() const {
        return std::string_view(data, N);
    }

    char data[N];
};
)";

static constexpr char const* const DESC_POST = R"(
template <class KernelType>
static constexpr auto get() {
    return get_impl<str_wrapper<__builtin_strlen(
        __builtin_sycl_unique_stable_name(std::remove_cvref_t<KernelType>))>(
        __builtin_sycl_unique_stable_name(std::remove_cvref_t<KernelType>))>();
}

}  // namespace __cham_sycl_kernel_descs

#define __cham_sycl_kernel_descs_DEFINED
}  // namespace
)";

struct transformer final {
    explicit transformer(clang::ASTContext& ast, std::ostream& desc)
        : info_(ast), desc_(desc) {}

    void transform(llvm::StringRef cxx_name, clang::Expr const* range,
                   clang::Expr const* offset, clang::Expr const* fn) {
        PUSH_CONTEXT(fn);

        if (auto const* dump = getenv("CSCC_DUMP_KERNEL"); dump && strcmp(dump, "1") == 0) {
            fn->dumpColor();
            fprintf(stderr, "-----------\n\n");
        }

        define_kernel_wrapper(range, offset, fn, cxx_name);
    }

    void finalize() {
        bool dump_desc = false;
        if (auto const* dump = getenv("CSCC_DUMP_DESC"); dump && strcmp(dump, "1") == 0) {
            dump_desc = true;
        }

        if (dump_desc) {
            fmt::print(stderr, "{}\n", desc_buffer_);
        }

        fmt::print(desc_, "{}\n", DESC_PRE);
        fmt::print(desc_, "{}\n", desc_buffer_);
        fmt::print(desc_, "template <auto Kernel> static constexpr auto get_impl() {{\n");
        fmt::print(desc_, "if constexpr (false) {{}}\n");
        for (auto const& [desc_name, kernel_name] : descmap_) {
            fmt::print(desc_,
                       "else if constexpr (Kernel.sv() == \"{}\") {{ return {}{{}}; }}\n",
                       kernel_name, desc_name);
            if (dump_desc) {
                fmt::print(stderr,
                           "else if constexpr (Kernel.sv() == \"{}\") {{ return {}{{}}; }}\n",
                           kernel_name, desc_name);
            }
        }
        fmt::print(desc_, "else {{\n");
        fmt::print(desc_, "Kernel.not_found(); }}\n");
        fmt::print(desc_, "}}\n");
        fmt::print(desc_, "{}\n", DESC_POST);

        if (dump_desc) {
            fmt::print(stderr, "-----------\n\n");
        }

        auto prg = info_.prg();
        prg->gensym_id = info_.nm().get_nextid();
        sort_decls(prg);
    }

    template <class Output>
    void dump_xml(Output& out) {
        struct writer : pugi::xml_writer {
            Output& out;

            explicit writer(Output& o) : out(o) {}

            void write(const void* data, size_t size) override {
                out.write(reinterpret_cast<char const*>(data), size);
            }
        } wr(out);

        pugi::xml_document doc;
        xcml::to_xml(doc, info_.prg());
        doc.save(wr, "  ");
    }

private:
    std::string const& define_kernel_wrapper(clang::Expr const* range,
                                             clang::Expr const* /*offset*/,
                                             clang::Expr const* fn, llvm::StringRef cxx_name) {
        PUSH_CONTEXT();

        using namespace xcml::utils;

        auto& ctx = info_.ctx();
        auto const size_t_ = info_.define_type(ctx.getSizeType());
        auto const unsigned_int_ptr_t_ =
            info_.define_type(ctx.getPointerType(ctx.UnsignedIntTy));
        auto wrapper = new_kernel_wrapper_decl();

        if (range) {
            auto const range_type_name = range->getType()->getAsCXXRecordDecl()->getName();
            if (range_type_name == llvm::StringRef("nd_range")) {
                wrapper->is_ndr = true;
            }
        }

        wrapper->name = "_s_";
        wrapper->name += std::string_view(cxx_name.data(), cxx_name.size());

        auto const* op = get_call_operator(info_, fn);
        auto const* record = op->getParent();
        auto const fn_record = ctx.getRecordType(record);
        auto const args = info_.nm().gen_var("args");
        auto const arg_ptr =
            add_local_var(wrapper->body, ctx.getPointerType(fn_record), info_.nm().this_name(),
                          make_addr_of(make_var_ref(args)));
        add_param(wrapper, info_.define_type(fn_record), args);

        auto const l = layout(ctx, fn_record, true);

        for (auto it = l.begin(); it != l.end(); ++it) {
            auto const type = it->decl()->getType();

            if (accessor_type acc_type;
                is_accessor(type, acc_type) && acc_type == accessor_type::DEVICE) {
                xcml::expr_ptr acc_ptr = arg_ptr;
                for (auto const* f : it->path()) {
                    acc_ptr = make_member_ref(acc_ptr, l.get_field_name(f));

                    if (!f->getType()->isPointerType()) {
                        acc_ptr = make_addr_of(acc_ptr);
                    }
                }

                auto const& ptr_param = info_.nm().gen_var("ptr");
                add_param(wrapper, info_.define_type(ctx.VoidPtrTy), ptr_param);

                auto ptr_ref = make_member_ref(acc_ptr, "ptr");
                push_expr(wrapper->body, assign_expr(ptr_ref, make_var_ref(ptr_param)));
            }
        }

        desc_buffer_.reserve(4096);
        auto const desc_name = fmt::format("desc_{}", descmap_.size());
        auto const kernel_name = std::string(cxx_name.data(), cxx_name.size());

        desc_buffer_ += fmt::format("struct {} {{\n", desc_name);
        desc_buffer_ += fmt::format("constexpr auto accessors() const {{\n");
        desc_buffer_ += fmt::format("return list<\n");
        unsigned int n_acc = 0;

        for (auto it = l.begin(); it != l.end(); ++it) {
            auto type = it->decl()->getType();
            type = ctx.getCanonicalType(type);

            if (accessor_type acc_type;
                is_accessor(type, acc_type) &&
                (acc_type == accessor_type::DEVICE || acc_type == accessor_type::LOCAL)) {
                if (n_acc) {
                    desc_buffer_ += fmt::format(", ");
                }

                std::string name;
                type.removeLocalFastQualifiers();
                auto const* cts = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(
                    type->getAsCXXRecordDecl());
                auto const dim = cts->getTemplateArgs()[1].getAsIntegral().getExtValue();

                if (acc_type == accessor_type::DEVICE) {
                    auto const mode = cts->getTemplateArgs()[2].getAsIntegral().getExtValue();
                    name = fmt::format(
                        "sycl::detail::device_accessor<dummy, {}, (sycl::access_mode){}>", dim,
                        mode);
                } else if (acc_type == accessor_type::LOCAL) {
                    name = fmt::format("sycl::local_accessor<dummy, {}>", dim);
                }

                desc_buffer_ += fmt::format("acc_desc<::{}, {}>{{}}", name, it->offset_of());

                n_acc++;
            }
        }

        desc_buffer_ += fmt::format(">();\n");
        desc_buffer_ += fmt::format("}}\n");
        desc_buffer_ += fmt::format("}};\n");
        descmap_.emplace_back(desc_name, kernel_name);

        if (op->getBody()) {
            auto _save = info_.scoped_set_captures(record);

            auto body = visit_compound_stmt(info_, op->getBody(), fn, op, true);

            push_stmt(wrapper->body, body);
        }

        info_.prg()->global_declarations.push_back(wrapper);

        return wrapper->name;
    }

    xcml::var_ref_ptr add_local_var(xcml::compound_stmt_ptr scope, clang::QualType type,
                                    std::string const& name, xcml::expr_ptr init = nullptr) {
        if (!type->isPointerType() && !type->isReferenceType()) {
            type.removeLocalConst();
        }

        return xcml::utils::add_local_var(scope, info_.define_type(type), name, init);
    }

    transform_info info_;
    std::ostream& desc_;
    std::string desc_buffer_;
    std::vector<std::pair<std::string, std::string>> descmap_;
};

static std::unique_ptr<transformer> g_transformer;

}  // namespace

void Transform(llvm::StringRef name, clang::ASTContext& ast, clang::Expr const* range,
               clang::Expr const* offset, clang::Expr const* fn, std::ostream& desc) {
    if (!g_transformer) {
        g_transformer = std::make_unique<transformer>(ast, desc);
    }
    g_transformer->transform(name, range, offset, fn);
}

void TransformSave(llvm::raw_ostream& out) {
    auto p = std::move(g_transformer);
    if (p) {
        p->finalize();
        p->dump_xml(out);
    }
}
