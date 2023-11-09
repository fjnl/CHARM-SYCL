#include "transform.hpp"
#include <cstdlib>
#include <optional>
#include <unordered_set>
#include <clang/AST/AST.h>
#include <clang/AST/StmtVisitor.h>
#include <fmt/format.h>
#include <pugixml.hpp>
#include <utils/naming.hpp>
#include <xcml.hpp>
#include <charm/sycl/fnv1a.hpp>
#include "error_trace.hpp"
#include "transform_info.hpp"
#include "utils.hpp"
#include "visitor.hpp"

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic warning "-Wall"
#    pragma GCC diagnostic warning "-Wextra"
#endif

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

clang::ClassTemplateSpecializationDecl* check_accessor_type(clang::QualType type,
                                                            bool* is_device, bool* is_local) {
    if (is_device) {
        *is_device = false;
    }
    if (is_local) {
        *is_local = false;
    }

    if (auto record = type->getAsCXXRecordDecl()) {
        if (record->getQualifiedNameAsString() == "sycl::accessor") {
            if (record->getNumBases() == 1) {
                auto const base = *record->bases_begin();
                auto const base_record = base.getType()->getAsCXXRecordDecl();
                auto const base_name = base_record->getQualifiedNameAsString();

                if (is_device) {
                    *is_device = (base_name == "sycl::detail::device_accessor");
                }
                if (is_local) {
                    *is_local = (base_name == "sycl::local_accessor");
                }
            }
        }

        return clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(record);
    }

    return nullptr;
}

clang::ClassTemplateSpecializationDecl* get_as_named_template(clang::QualType qty,
                                                              char const* name) {
    if (auto record = qty->getAsRecordDecl()) {
        if (record->getQualifiedNameAsString() == name) {
            return clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(record);
        }
    }
    return nullptr;
}

struct accessor_type {
    clang::QualType origin;
    clang::QualType data_type;
    int dimensions;
    int access_mode;
    int target;
};

std::optional<accessor_type> get_as_accessor(clang::QualType qty) {
    bool is_device;
    if (auto const* tsd = check_accessor_type(qty, &is_device, nullptr); is_device) {
        auto const& args = tsd->getTemplateArgs();

        assert(args.size() == 4);

        accessor_type at;
        at.origin = qty;
        at.data_type = args.get(0).getAsType();
        at.dimensions = args.get(1).getAsIntegral().getExtValue();
        at.access_mode = args.get(2).getAsIntegral().getExtValue();
        at.target = args.get(3).getAsIntegral().getExtValue();
        return at;
    }

    return std::nullopt;
}

struct id_type {
    clang::QualType origin;
    int dimensions;
};

std::optional<id_type> get_as_id(clang::QualType qty) {
    if (auto tsd = get_as_named_template(qty, "sycl::id")) {
        auto const& args = tsd->getTemplateArgs();

        assert(args.size() == 1);

        id_type it;
        it.origin = qty;
        it.dimensions = args.get(0).getAsIntegral().getExtValue();

        return it;
    }

    return std::nullopt;
}

struct range_type {
    clang::QualType origin;
    int dimensions;
};

std::optional<range_type> get_as_range(clang::QualType qty) {
    if (auto tsd = get_as_named_template(qty, "sycl::range")) {
        auto const& args = tsd->getTemplateArgs();

        assert(args.size() == 1);

        range_type rt;
        rt.origin = qty;
        rt.dimensions = args.get(0).getAsIntegral().getExtValue();

        return rt;
    }

    return std::nullopt;
}

struct nd_range_type {
    clang::QualType origin;
    int dimensions;
};

std::optional<nd_range_type> get_as_nd_range(clang::QualType qty) {
    if (auto tsd = get_as_named_template(qty, "sycl::nd_range")) {
        auto const& args = tsd->getTemplateArgs();

        assert(args.size() == 1);

        nd_range_type rt;
        rt.origin = qty;
        rt.dimensions = args.get(0).getAsIntegral().getExtValue();

        return rt;
    }

    return std::nullopt;
}

struct item_type {
    clang::QualType origin;
    int dimensions;
};

std::optional<item_type> get_as_item(clang::QualType qty) {
    if (auto tsd = get_as_named_template(qty, "sycl::item")) {
        auto const& args = tsd->getTemplateArgs();

        assert(args.size() == 2);

        item_type t;
        t.origin = qty;
        t.dimensions = args.get(0).getAsIntegral().getExtValue();

        return t;
    }

    return std::nullopt;
}

struct nd_item_type {
    clang::QualType origin;
    int dimensions;
};

std::optional<nd_item_type> get_as_nd_item(clang::QualType qty) {
    if (auto tsd = get_as_named_template(qty, "sycl::nd_item")) {
        auto const& args = tsd->getTemplateArgs();

        assert(args.size() == 1);

        nd_item_type t;
        t.origin = qty;
        t.dimensions = args.get(0).getAsIntegral().getExtValue();

        return t;
    }

    return std::nullopt;
}

template <class T>
struct scoped_set {
    template <class U>
    explicit scoped_set(T& ref, U const& new_) : ref_(ref), old_(ref) {
        ref = new_;
    }

    scoped_set(scoped_set const&) = delete;

    scoped_set(scoped_set&&) = delete;

    scoped_set& operator=(scoped_set const&) = delete;

    scoped_set& operator=(scoped_set&&) = delete;

    ~scoped_set() {
        ref_ = old_;
    }

private:
    T& ref_;
    T old_;
};

struct transformer final {
    explicit transformer(clang::ASTContext& ast) : info_(ast) {}

    void transform(llvm::StringRef cxx_name, clang::Expr const* range,
                   clang::Expr const* offset, clang::Expr const* fn) {
        PUSH_CONTEXT(fn);

        bool is_ndr = false;

        auto call = define_kernel(fn, is_ndr);
        define_kernel_wrapper(range, offset, fn, cxx_name, call, is_ndr);
    }

    void synchronize() {
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
    xcml::function_call_ptr define_kernel(clang::Expr const* fn, bool& is_ndr) {
        PUSH_CONTEXT(fn);

        function_builder f(info_, info_.nm().gen_func("kernel"));
        f.set_static();
        is_ndr = false;

        auto op = get_call_operator(fn);

        if (op->param_size() != 0 && op->param_size() != 1) {
            not_supported(op, info_.ctx(), "number of arguments are more than 1");
        }

        if (op->param_size() > 0) {
            auto const* iter = op->getParamDecl(0);
            auto iter_type = iter->getType();
            remove_const_lvref(iter_type);
            auto const need_stub = iter->getType()->isReferenceType();
            auto const need_declare = !iter->getName().empty();
            auto const iter_name = info_.rename_sym(iter);

            std::vector<xcml::var_ref_ptr> indices, ranges;

            auto const add_params = [&](int ndim, bool is_ndr) {
                auto const add = [&](auto& vec, int n, char const* pre) {
                    for (int dim = 0; dim < n; ++dim) {
                        auto const name = info_.nm().gen_var(fmt::format("{}{}", pre, dim));
                        vec.push_back(f.add_param(info_.ctx().getSizeType(), name));
                    }
                };

                if (is_ndr) {
                    add(indices, ndim, "group");
                    add(indices, ndim, "local");
                    add(ranges, ndim, "group_range");
                    add(ranges, ndim, "local_range");
                } else {
                    add(indices, ndim, "id");
                    add(ranges, ndim, "range");
                }
            };

            if (auto id = get_as_id(iter_type)) {
                add_params(id->dimensions, false);

                if (need_declare) {
                    xcml::expr_ptr id_var, id_addr;

                    if (need_stub) {
                        id_var = add_local_var(f.body(), id->origin, info_.nm().gen_var("id"));
                        id_addr =
                            add_local_var(f.body(), info_.ctx().getPointerType(id->origin),
                                          iter_name, make_addr_of(id_var));

                    } else {
                        id_var = add_local_var(f.body(), id->origin, iter_name);
                        id_addr = make_addr_of(id_var);
                    }

                    for (int dim = 0; dim < id->dimensions; ++dim) {
                        auto asg = assign_expr(
                            make_array_ref(make_member_ref(id_addr, "id_"), lit(dim)),
                            indices.at(dim));

                        push_expr(f.body(), asg);
                    }
                }
            } else if (auto item = get_as_item(iter_type)) {
                add_params(item->dimensions, false);

                if (need_declare) {
                    xcml::expr_ptr item_var, item_addr;

                    if (need_stub) {
                        item_var =
                            add_local_var(f.body(), item->origin, info_.nm().gen_var("item"));
                        item_addr =
                            add_local_var(f.body(), info_.ctx().getPointerType(item->origin),
                                          iter_name, make_addr_of(item_var));

                    } else {
                        item_var = add_local_var(f.body(), item->origin, iter_name);
                        item_addr = make_addr_of(item_var);
                    }

                    for (int dim = 0; dim < item->dimensions; ++dim) {
                        auto asg = assign_expr(
                            make_array_ref(
                                make_member_ref(make_member_addr(item_addr, "id_"), "id_"),
                                lit(dim)),
                            indices.at(dim));

                        push_expr(f.body(), asg);
                    }
                    for (int dim = 0; dim < item->dimensions; ++dim) {
                        auto asg = assign_expr(
                            make_array_ref(make_member_ref(
                                               make_member_addr(item_addr, "range_"), "range_"),
                                           lit(dim)),
                            ranges.at(dim));

                        push_expr(f.body(), asg);
                    }
                }
            } else if (auto nd_item = get_as_nd_item(iter_type)) {
                auto const ndim = nd_item->dimensions;
                add_params(ndim, true);

                if (need_declare) {
                    xcml::expr_ptr item_var, item_addr;

                    if (need_stub) {
                        item_var = add_local_var(f.body(), nd_item->origin,
                                                 info_.nm().gen_var("item"));
                        item_addr =
                            add_local_var(f.body(), info_.ctx().getPointerType(nd_item->origin),
                                          iter_name, make_addr_of(item_var));

                    } else {
                        item_var = add_local_var(f.body(), nd_item->origin, iter_name);
                        item_addr = make_addr_of(item_var);
                    }

                    for (int dim = 0; dim < ndim; ++dim) {
                        auto asg = assign_expr(
                            make_array_ref(
                                make_member_ref(make_member_addr(item_addr, "group_"),
                                                "group_id_"),
                                lit(dim)),
                            indices.at(dim));

                        push_expr(f.body(), asg);
                    }
                    for (int dim = 0; dim < ndim; ++dim) {
                        auto asg = assign_expr(
                            make_array_ref(
                                make_member_ref(make_member_addr(item_addr, "group_"),
                                                "local_id_"),
                                lit(dim)),
                            indices.at(ndim + dim));

                        push_expr(f.body(), asg);
                    }
                    for (int dim = 0; dim < ndim; ++dim) {
                        auto asg = assign_expr(
                            make_array_ref(
                                make_member_ref(make_member_addr(item_addr, "group_"),
                                                "group_range_"),
                                lit(dim)),
                            ranges.at(dim));

                        push_expr(f.body(), asg);
                    }
                    for (int dim = 0; dim < ndim; ++dim) {
                        auto asg = assign_expr(
                            make_array_ref(
                                make_member_ref(make_member_addr(item_addr, "group_"),
                                                "local_range_"),
                                lit(dim)),
                            ranges.at(ndim + dim));

                        push_expr(f.body(), asg);
                    }
                }

                is_ndr = true;
            } else {
                not_supported(iter_type, info_.ctx(), "not supported iterator type");
            }
        }

        for (auto [vd, vf] : captured_vars(info_, fn)) {
            f.add_param(vf->getType(), info_.rename_sym(vd));
        }

        if (op->getBody()) {
            auto body = visit_compound_stmt(info_, op->getBody(), fn, op, true);

            push_stmt(f.body(), body);
        }

        f.body();
        f.set_inline(false);

        return f.call_expr();
    }

    std::string const& define_kernel_wrapper(clang::Expr const* range,
                                             clang::Expr const* offset, clang::Expr const* fn,
                                             llvm::StringRef cxx_name,
                                             xcml::function_call_ptr call, bool is_ndr) {
        PUSH_CONTEXT();

        using namespace xcml::utils;

        auto& ctx = info_.ctx();
        auto const size_t_ = info_.define_type(ctx.getSizeType());
        auto const unsigned_int_ptr_t_ =
            info_.define_type(ctx.getPointerType(ctx.UnsignedIntTy));
        auto wrapper = new_kernel_wrapper_decl();

        wrapper->name = "_s_";
        wrapper->name += std::string_view(cxx_name.data(), cxx_name.size());

        if (is_ndr) {
            for (int dim = 0; dim < 3; ++dim) {
                add_param(wrapper, size_t_,
                          info_.nm().gen_var(fmt::format("global_range{}", dim)));
            }
            for (int dim = 0; dim < 3; ++dim) {
                add_param(wrapper, size_t_,
                          info_.nm().gen_var(fmt::format("local_range{}", dim)));
            }
            add_param(wrapper, unsigned_int_ptr_t_, "_s__lmem_aux");
        } else {
            for (int dim = 0; dim < 3; ++dim) {
                add_param(wrapper, size_t_, info_.nm().gen_var(fmt::format("range{}", dim)));
            }
            if (offset) {
                for (int dim = 0; dim < 3; ++dim) {
                    add_param(wrapper, size_t_,
                              info_.nm().gen_var(fmt::format("offset{}", dim)));
                }
            }
        }

        auto captures = captured_vars(info_, fn);

        for (auto [vd, fd] : captures) {
            auto type = fd->getType();
            if (!type->isPointerType() && !type->isReferenceType()) {
                type.removeLocalConst();
            }

            auto const value_type = info_.define_type(type);
            auto const& name = info_.rename_sym(vd);

            if (auto const acc = get_as_accessor(type)) {
                auto const ptr_type =
                    info_.define_type(info_.ctx().getPointerType(acc->data_type));
                add_param(wrapper, ptr_type, info_.nm().gen_var("ptr"));
                wrapper->memories.insert(wrapper->params.size() - 1);
            }
            add_param(wrapper, value_type, name);
        }

        auto const f = [&](auto par, size_t ndim) {
            for (size_t dim = 0; dim < ndim; ++dim) {
                auto param = xcml::param_node::dyncast(wrapper->params.at(3 - ndim + dim));

                par->arguments.push_back(make_var_ref(param->name));
            }
            if (is_ndr) {
                for (size_t dim = 0; dim < ndim; ++dim) {
                    auto param = xcml::param_node::dyncast(wrapper->params.at(6 - ndim + dim));

                    par->arguments.push_back(make_var_ref(param->name));
                }
            }

            par->function = call->function;

            for (auto [vd, _] : captures) {
                par->arguments.push_back(make_var_ref(info_.rename_sym(vd)));
            }

            wrapper->body = par;
        };

        if (is_ndr) {
            auto invoke = make_ndr_invoke(range, wrapper);
            assert(invoke->group.size() == invoke->local.size());
            f(invoke, invoke->group.size());
        } else {
            auto invoke = make_paralell_invoke(range, offset, wrapper);
            f(invoke, invoke->dimensions.size());
        }

        info_.prg()->global_declarations.push_back(wrapper);

        return wrapper->name;
    }

    clang::CXXMethodDecl const* get_call_operator(clang::CXXRecordDecl const* record) const {
        PUSH_CONTEXT(record);

        if (auto const* cts = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(record)) {
            record = cts;
        }

        for (auto const& method : record->methods()) {
            // TODO: consider overloading
            if (method->getOverloadedOperator() == clang::OO_Call) {
                return method;
            }
        }

        not_supported(record, info_.ctx(), "unable to find the call operator");
    }

    clang::CXXMethodDecl const* get_call_operator(clang::Expr const* fn) const {
        PUSH_CONTEXT(fn);

        if (auto const* lambda = clang::dyn_cast<clang::LambdaExpr>(fn)) {
            return lambda->getCallOperator();
        }

        if (auto const* dr = clang::dyn_cast<clang::DeclRefExpr>(fn)) {
            return get_call_operator(dr->getDecl()->getType()->getAsCXXRecordDecl());
        }

        if (auto const* ctor = clang::dyn_cast<clang::CXXConstructExpr>(fn)) {
            return get_call_operator(ctor->getType()->getAsCXXRecordDecl());
        }

        not_supported(fn, info_.ctx(), "unable to find the call operator");
    }

    xcml::parallel_invoke_ptr make_paralell_invoke(
        clang::Expr const* range_expr, clang::Expr const* offset_expr,
        xcml::kernel_wrapper_decl_ptr const& wrapper) {
        if (range_expr) {
            auto type = range_expr->getType();
            auto type_ref = info_.define_type(type);
            auto ptr_ref = info_.define_type(info_.ctx().getPointerType(type));

            if (auto range = get_as_range(type)) {
                auto par = xcml::new_parallel_invoke();

                for (int dim = 3 - range->dimensions; dim < 3; ++dim) {
                    auto d = xcml::new_dimension();
                    auto param = xcml::param_node::dyncast(wrapper->params.at(dim));

                    d->iter = info_.nm().gen_var(fmt::format("kernel_iter{}", dim));
                    d->type = info_.define_type(info_.ctx().getSizeType());
                    if (offset_expr) {
                        auto off_param = xcml::param_node::dyncast(wrapper->params.at(3 + dim));

                        clang::Expr::EvalResult res;
                        if (offset_expr->EvaluateAsRValue(res, info_.ctx()) &&
                            !res.HasUndefinedBehavior && res.Val.isInt()) {
                            auto const& val = res.Val.getInt();

                            if (val.isSignedIntN(64)) {
                                d->offset = lit(val.getSExtValue());
                            } else if (val.isIntN(64)) {
                                d->offset = lit(val.getExtValue());
                            }
                        }

                        if (!d->offset) {
                            d->offset = make_var_ref(off_param->name);
                        }
                        d->size =
                            plus_expr(make_var_ref(param->name), make_var_ref(off_param->name));
                    } else {
                        d->offset = lit(0);
                        d->size = make_var_ref(param->name);
                    }
                    par->dimensions.push_back(d);
                }

                return par;
            }
        } else {
            return xcml::new_parallel_invoke();
        }

        not_supported(range_expr, info_.ctx());
    }

    xcml::ndr_invoke_ptr make_ndr_invoke(clang::Expr const* range_expr,
                                         xcml::kernel_wrapper_decl_ptr const& wrapper) {
        if (range_expr) {
            auto type = range_expr->getType();

            if (auto nd_range = get_as_nd_range(type)) {
                auto par = xcml::new_ndr_invoke();
                auto const ndim = nd_range->dimensions;
                auto const& size_type = info_.define_type(info_.ctx().getSizeType());

                for (int dim = 3 - ndim; dim < 3; ++dim) {
                    auto d = xcml::new_dimension();
                    auto param = xcml::param_node::dyncast(wrapper->params.at(dim));

                    d->iter = info_.nm().gen_var(fmt::format("group_iter{}", dim));
                    d->type = size_type;
                    d->offset = lit(0);
                    d->size = make_var_ref(param->name);
                    par->group.push_back(d);
                }

                for (int dim = 3 - ndim; dim < 3; ++dim) {
                    auto d = xcml::new_dimension();
                    auto param = xcml::param_node::dyncast(wrapper->params.at(3 + dim));

                    d->iter = info_.nm().gen_var(fmt::format("local_iter{}", dim));
                    d->type = size_type;
                    d->offset = lit(0);
                    d->size = make_var_ref(param->name);
                    par->local.push_back(d);
                }

                return par;
            }
        }

        not_supported(range_expr, info_.ctx());
    }

    xcml::var_ref_ptr add_local_var(xcml::compound_stmt_ptr scope, clang::QualType type,
                                    std::string const& name, xcml::expr_ptr init = nullptr) {
        if (!type->isPointerType() && !type->isReferenceType()) {
            type.removeLocalConst();
        }

        return xcml::utils::add_local_var(scope, info_.define_type(type), name, init);
    }

    transform_info info_;
};

static std::unique_ptr<transformer> g_transformer;

}  // namespace

void Transform(llvm::StringRef name, clang::ASTContext& ast, clang::Expr const* range,
               clang::Expr const* offset, clang::Expr const* fn) {
    if (!g_transformer) {
        g_transformer = std::make_unique<transformer>(ast);
    }
    g_transformer->transform(name, range, offset, fn);
}

void TransformSave(llvm::raw_ostream& out) {
    auto p = std::move(g_transformer);
    if (p) {
        p->synchronize();
        p->dump_xml(out);
    }
}
