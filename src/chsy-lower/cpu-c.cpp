#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utils/hash.hpp>
#include <utils/target.hpp>
#include <xcml.hpp>
#include <xcml_recusive_visitor.hpp>
#include "chsy-lower.hpp"
#include "math.hpp"

namespace {

namespace u = xcml::utils;

static char const* const LMEM_VAR = "_s__lmem";
static char const* const WG_VAR = "_s__wg";
static char const* const LMEM_AUX_VAR = "_s__lmem_aux";
using funcset_t = std::unordered_set<std::string>;
using callmap_t = std::unordered_multimap<std::string, std::string>;

struct add_function_loader_visitor final
    : xcml::recursive_visitor<add_function_loader_visitor> {
    explicit add_function_loader_visitor(utils::target t) : target_(t) {}

    xcml::node_ptr visit_kernel_wrapper_decl(xcml::kernel_wrapper_decl_ptr const& node,
                                             scope_ref) {
        using namespace xcml::utils;

        auto ft = create_function_type();
        ft->return_type = "void";

        auto const name = gen_func("function_loader");
        fdecl_opts opts;
        opts.is_static = true;

        auto fa = add_fdecl(root(), ft, name, opts);
        auto fd = make_fd(root(), ft, name);

        auto attr = new_gcc_attribute();
        attr->name = "constructor";
        fd->attributes.push_back(std::move(attr));

        auto void_ptr = get_pointer_type(get_basic_type("void"));
        auto target_str = show(target_);

        auto call = xcml::new_function_call();
        call->function = get_registry_fn();
        call->arguments.push_back(lit(node->name));
        call->arguments.push_back(lit(utils::fnv1a(node->name.c_str())));
        call->arguments.push_back(lit(target_str));
        call->arguments.push_back(lit(utils::fnv1a(target_str)));
        call->arguments.push_back(make_cast(void_ptr, make_func_addr(node->name)));

        push_expr(fd->body, call);

        return node;
    }

private:
    xcml::func_addr_ptr const& get_registry_fn() {
        using namespace xcml::utils;

        if (!registry_fn_) {
            auto ft = create_function_type();
            ft->return_type = "void";

            auto char_const_ptr = get_pointer_type(get_basic_type("char"));
            auto void_ptr = get_pointer_type(get_basic_type("void"));
            auto unsigned_long = get_basic_type("unsigned_long");

            add_param(ft, char_const_ptr, "name");
            add_param(ft, unsigned_long, "name_hash");
            add_param(ft, char_const_ptr, "kind");
            add_param(ft, unsigned_long, "kind_hash");
            add_param(ft, void_ptr, "f");

            fdecl_opts opts;
            opts.extern_c = true;
            registry_fn_ = add_fdecl(root(), ft, "__s_add_kernel_registry", opts);
        }

        return registry_fn_;
    }

    utils::target target_;
    xcml::func_addr_ptr registry_fn_;
};

struct transform_kernel_wrapper final : xcml::recursive_visitor<transform_kernel_wrapper> {
    xcml::node_ptr visit_kernel_wrapper_decl(xcml::kernel_wrapper_decl_ptr const& node,
                                             scope_ref) {
        using namespace xcml::utils;

        auto ft = create_function_type();
        ft->return_type = "void";
        auto const args = add_param(ft, get_void_ptr_ptr(), gen_var("args"));

        fdecl_opts opts;
        opts.is_static = true;

        auto const fa = add_fdecl(root(), ft, node->name, opts);
        auto const fd = make_fd(root(), ft, node->name, true);

        for (size_t i = 0; i < node->params.size(); i++) {
            auto const& p = node->params.at(i);
            auto param = xcml::param_node::dyncast(p);

            if (!param) {
                continue;
            }

            auto const& type = type_map_.at(param->type);
            auto const ptr_type = get_pointer_type(type);
            auto const value = make_cast(ptr_type, make_array_ref(args, lit(i)));

            auto const is_memory = !!node->memories.count(i);
            auto const is_struct = xcml::struct_type::is_a(type);

            if (is_memory) {
                // args[i] = T*, args[i+1] = rts::accessor
                auto const ptr =
                    add_local_var(fd->body, type, gen_var("ptr"), make_deref(value));
                auto const range = add_local_var(
                    fd->body, rts_accessor_ptr_type(), gen_var("range"),
                    make_cast(rts_accessor_ptr_type(), make_array_ref(args, lit(i + 1))));

                auto const& p2 = node->params.at(i + 1);
                auto const& param2 = xcml::param_node::dyncast(p2);
                auto const param2_type = type_map_.at(param2->type);
                auto const acc0 = add_local_var(fd->body, param2_type, gen_var("acc"));
                auto const acc = add_local_var(fd->body, get_pointer_type(param2_type),
                                               param2->name, make_addr_of(acc0));

                push_expr(fd->body, assign_expr(make_member_ref(acc, "ptr"), ptr));
                push_expr(fd->body, assign_expr(make_member_ref(acc, "size0"),
                                                make_member_ref(range, "size0")));
                push_expr(fd->body, assign_expr(make_member_ref(acc, "size1"),
                                                make_member_ref(range, "size1")));
                push_expr(fd->body, assign_expr(make_member_ref(acc, "size2"),
                                                make_member_ref(range, "size2")));
                push_expr(fd->body, assign_expr(make_member_ref(acc, "offset0"),
                                                make_member_ref(range, "offset0")));
                push_expr(fd->body, assign_expr(make_member_ref(acc, "offset1"),
                                                make_member_ref(range, "offset1")));
                push_expr(fd->body, assign_expr(make_member_ref(acc, "offset2"),
                                                make_member_ref(range, "offset2")));

                i++;
            } else if (is_struct) {
                add_local_var(fd->body, ptr_type, param->name, value);
            } else {
                add_local_var(fd->body, type, param->name, make_deref(value));
            }
        }

        fd->body->body.push_back(node->body);

        return fd;
    }

private:
    xcml::type_ptr get_void_ptr() {
        if (auto t = find_pointer_type(get_basic_type("void"))) {
            return t;
        }

        return create_pointer_type("void");
    }

    xcml::type_ptr get_void_ptr_ptr() {
        if (auto t = find_pointer_type(get_void_ptr())) {
            return t;
        }
        return create_pointer_type(get_void_ptr());
    }
};

struct transform_parallel_for final : xcml::recursive_visitor<transform_parallel_for> {
    explicit transform_parallel_for(utils::target t, funcset_t const& barrier_funcs,
                                    std::unordered_set<std::string>* par_kernels,
                                    std::unordered_map<std::string, int>* ndr_kernels)
        : target_(t),
          barrier_funcs_(barrier_funcs),
          par_kernels_(par_kernels),
          ndr_kernels_(ndr_kernels) {}

    xcml::node_ptr visit_parallel_invoke(xcml::parallel_invoke_ptr const& node, scope_ref) {
        using namespace xcml::utils;

        auto top = xcml::new_compound_stmt();
        auto scope = top;
        std::vector<xcml::expr_ptr> args;

        for (auto const& dim : node->dimensions) {
            auto iter = add_local_var(scope, dim->type, dim->iter);
            auto for_ = xcml::new_for_stmt();

            for_->init = assign_expr(iter, dim->offset);
            for_->condition = log_lt_expr(iter, dim->size);
            for_->iter = pre_incr_expr(iter);

            if (top == scope && is_openmp(target_)) {
                for_->pragma.push_back(make_pragma(
                    fmt::format("omp parallel for collapse({})", node->dimensions.size())));
            }

            args.push_back(iter);
            scope->body.push_back(for_);
            scope = for_->body;
        }

        args.insert(args.end(), node->arguments.begin(), node->arguments.end());

        auto fa = xcml::func_addr::dyncast(node->function);
        auto call = make_call(fa, args.begin(), args.end());

        push_expr(scope, call);

        if (par_kernels_) {
            par_kernels_->insert(fa->name);
        }

        return top;
    }

    xcml::node_ptr visit_ndr_invoke(xcml::ndr_invoke_ptr const& node, scope_ref) {
        using namespace xcml::utils;

        if (!decled_) {
            decl_fiber_barrier();
            decl_fiber_fork();
            decled_ = true;
        }

        auto top = xcml::new_compound_stmt();
        auto scope = top;
        std::vector<xcml::expr_ptr> args;

        for (auto const& dim : node->group) {
            auto iter = add_local_var(scope, dim->type, dim->iter);
            auto for_ = xcml::new_for_stmt();

            for_->init = assign_expr(iter, dim->offset);
            for_->condition = log_lt_expr(iter, dim->size);
            for_->iter = pre_incr_expr(iter);

            if (top == scope && is_openmp(target_)) {
                for_->pragma.push_back(make_pragma(
                    fmt::format("omp parallel for collapse({})", node->group.size())));
            }

            args.push_back(iter);
            scope->body.push_back(for_);
            scope = for_->body;
        }

        if (requires_fiber(node)) {
            insert_fiber_fork(node, args, scope);
        } else {
            insert_local_loops(node, args, scope);
        }

        if (ndr_kernels_) {
            auto fa = xcml::func_addr::dyncast(node->function);
            ndr_kernels_->insert_or_assign(fa->name, node->group.size());
        }

        return top;
    }

private:
    bool requires_fiber(xcml::ndr_invoke_ptr const& node) const {
        return barrier_funcs_.find(xcml::func_addr::dyncast(node->function)->name) !=
               barrier_funcs_.end();
    }

    void insert_local_loops(xcml::ndr_invoke_ptr const& node, std::vector<xcml::expr_ptr>& args,
                            xcml::compound_stmt_ptr scope) {
        for (auto const& dim : node->local) {
            auto iter = u::add_local_var(scope, dim->type, dim->iter);
            auto for_ = xcml::new_for_stmt();

            for_->init = u::assign_expr(iter, dim->offset);
            for_->condition = u::log_lt_expr(iter, dim->size);
            for_->iter = u::pre_incr_expr(iter);

            args.push_back(iter);
            scope->body.push_back(for_);
            scope = for_->body;
        }

        args.insert(args.end(), node->arguments.begin(), node->arguments.end());

        auto fa = xcml::func_addr::dyncast(node->function);
        auto call = u::make_call(fa, args.begin(), args.end());

        u::push_expr(scope, call);
    }

    void insert_fiber_fork(xcml::ndr_invoke_ptr const& node, std::vector<xcml::expr_ptr>& args,
                           xcml::compound_stmt_ptr const& scope) {
        // dim
        args.insert(args.begin(), u::lit(static_cast<int>(node->group.size())));

        size_t p = 1 + node->group.size();

        // group iter padding
        for (size_t i = node->group.size(); i < 3; i++) {
            args.insert(args.begin() + p++, u::lit(0));
        }

        // local ranges
        for (auto const& dim : node->local) {
            args.insert(args.begin() + p++, dim->size);
        }

        // local range padding
        for (size_t i = node->local.size(); i < 3; i++) {
            args.insert(args.begin() + p++, u::lit(1));
        }

        // size of local memory
        args.insert(args.begin() + p++, u::make_var_ref(LMEM_AUX_VAR));

        // kernel function pointer
        args.insert(args.begin() + p++, node->function);

        // forward actual arguments
        args.insert(args.end(), node->arguments.begin(), node->arguments.end());

        u::push_expr(scope, u::make_call(u::make_func_addr("__charm_sycl_fiber_fork"),
                                         args.begin(), args.end()));
    }

    void decl_fiber_barrier() {
        auto ft = create_function_type();

        ft->return_type = "void";

        u::add_param(ft, get_pointer_type(get_basic_type("void")), gen_var("wg"));

        u::fdecl_opts opts;
        opts.extern_c = true;
        u::add_fdecl(root(), ft, "__charm_sycl_fiber_barrier", opts);
    }

    void decl_fiber_fork() {
        auto ft = create_function_type();

        ft->return_type = "void";

        u::add_param(ft, "int", gen_var("ndim"));
        for (int i = 0; i < 3; i++) {
            u::add_param(ft, "unsigned_long", gen_var("g"));
        }
        for (int i = 0; i < 3; i++) {
            u::add_param(ft, "unsigned_long", gen_var("l_range"));
        }
        u::add_param(ft, get_pointer_type(get_basic_type("unsigned")), gen_var("lmem_size"));
        u::add_param(ft, get_pointer_type(get_basic_type("void")), gen_var("fn"));

        ft->params.push_back(xcml::new_ellipsis());

        u::fdecl_opts opts;
        opts.extern_c = true;
        u::add_fdecl(root(), ft, "__charm_sycl_fiber_fork", opts);
    }

    utils::target target_;
    funcset_t const& barrier_funcs_;
    std::unordered_set<std::string>* par_kernels_ = nullptr;
    std::unordered_map<std::string, int>* ndr_kernels_ = nullptr;
    bool decled_ = false;
};

struct build_callmap_visitor final : xcml::recursive_visitor<build_callmap_visitor> {
    using parent = xcml::recursive_visitor<build_callmap_visitor>;
    xcml::node_ptr visit_function_definition(xcml::function_definition_ptr const& node,
                                             scope_ref scope) {
        assert(!fd_);

        fd_ = node;
        auto res = parent::visit_function_definition(node, scope);
        fd_ = nullptr;

        return res;
    }

    xcml::node_ptr visit_function_call(xcml::function_call_ptr const& node, scope_ref scope) {
        assert(fd_);

        auto fn = xcml::func_addr::dyncast(node->function);
        assert(fn);

        call_map_.emplace(fd_->name, fn->name);

        return parent::visit_function_call(node, scope);
    }

    callmap_t const& get() const {
        return call_map_;
    }

private:
    xcml::function_definition_ptr fd_;
    callmap_t call_map_;
};

callmap_t build_callmap(xcml::xcml_program_node_ptr const& prg) {
    build_callmap_visitor vis;
    vis(prg);
    return vis.get();
}

funcset_t transitive_closure(callmap_t const& cm, std::string const& start) {
    callmap_t rev;

    for (auto const& [k, v] : cm) {
        rev.emplace(v, k);
    }

    funcset_t funcs;
    std::queue<std::string> q;
    q.push(start);

    while (!q.empty()) {
        auto range = rev.equal_range(q.front());

        for (auto it = range.first; it != range.second; ++it) {
            if (funcs.insert(it->second).second) {
                q.push(it->second);
            }
        }

        q.pop();
    }

    return funcs;
}

funcset_t build_lmem_consumers(callmap_t const& cm) {
    return transitive_closure(cm, "__charm_sycl_local_memory_base");
}

funcset_t build_barrier_consumers(callmap_t const& cm) {
    return transitive_closure(cm, "__charm_sycl_group_barrier");
}

struct rewrite_args_visitor final : xcml::recursive_visitor<rewrite_args_visitor> {
    using parent = xcml::recursive_visitor<rewrite_args_visitor>;

    explicit rewrite_args_visitor(funcset_t const& lmem_funcs, funcset_t const& barrier_funcs)
        : lmem_funcs_(lmem_funcs), barrier_funcs_(barrier_funcs) {}

    xcml::xcml_program_node_ptr operator()(xcml::xcml_program_node_ptr const& node) {
        return (*static_cast<parent*>(this))(node);
    }

    xcml::node_ptr visit_function_definition(xcml::function_definition_ptr const& node,
                                             scope_ref scope) {
        auto const lmem = is_lmem(node->name);
        auto const barrier = is_barrier(node->name);

        if (lmem || barrier) {
            auto ft = lookup(scope, node->name).function_type();

            if (lmem) {
                u::add_param(ft, node, get_pointer_type(get_basic_type("void")), LMEM_VAR);
            }
            if (barrier) {
                u::add_param(ft, node, get_pointer_type(get_basic_type("void")), WG_VAR);
            }
        }

        return parent::visit_function_definition(node, scope);
    }

    xcml::node_ptr visit_function_call(xcml::function_call_ptr node, scope_ref scope) {
        node = xcml::function_call::dyncast(parent::visit_function_call(node, scope));

        auto fa = xcml::func_addr::dyncast(node->function);
        if (is_lmem(fa->name)) {
            node->arguments.push_back(u::make_var_ref(LMEM_VAR));
            modified_ = true;
        }
        if (is_barrier(fa->name)) {
            node->arguments.push_back(u::make_var_ref(WG_VAR));
            modified_ = true;
        }

        return node;
    }

    bool modified() const {
        return modified_;
    }

private:
    bool is_lmem(std::string const& name) const {
        return lmem_funcs_.find(name) != lmem_funcs_.end();
    }

    bool is_barrier(std::string const& name) const {
        return barrier_funcs_.find(name) != barrier_funcs_.end();
    }

    funcset_t const& lmem_funcs_;
    funcset_t const& barrier_funcs_;
    bool modified_ = false;
};

xcml::function_type_ptr get_function_type(xcml::xcml_program_node_ptr prg,
                                          std::string const& name) {
    for (auto const& sym : prg->global_symbols) {
        if (sym->name == name) {
            return u::get_function_type(prg, sym->type);
        }
    }

    return nullptr;
}

xcml::basic_type_ptr get_or_create_va_list_type(xcml::xcml_program_node_ptr const& prg) {
    for (auto const& type : prg->type_table) {
        if (auto bt = xcml::basic_type::dyncast(type); bt && bt->name == "va_list") {
            return bt;
        }
    }

    auto bt = xcml::new_basic_type();
    bt->name = "va_list";
    bt->type = "va_list";
    bt->is_builtin = true;

    prg->type_table.push_back(bt);

    return bt;
}

xcml::type_ptr get_or_create_voidptr_type(xcml::xcml_program_node_ptr const& prg) {
    for (auto const& type : prg->type_table) {
        if (auto pt = xcml::pointer_type::dyncast(type); pt && pt->ref == "void") {
            return pt;
        }
    }

    utils::naming_utils nm(prg->gensym_id);

    auto pt = xcml::new_pointer_type();
    pt->ref = "void";
    pt->type = nm.xptr_type();

    prg->gensym_id = nm.get_nextid();

    return pt;
}

void rewrite_kernel_args(xcml::xcml_program_node_ptr prg,
                         std::unordered_map<std::string, int> const& ndr_kernels) {
    auto inc = u::new_cpp_include();
    inc->name = "stdarg.h";
    prg->global_declarations.push_front(inc);

    for (auto const& decl : prg->global_declarations) {
        if (auto fd = xcml::function_definition::dyncast(decl)) {
            auto const it = ndr_kernels.find(fd->name);
            if (it == ndr_kernels.end()) {
                continue;
            }

            auto ft = get_function_type(prg, fd->name);
            auto va_list_type = get_or_create_va_list_type(prg);
            auto& body = fd->body;

            auto ap = u::make_var_ref("_s__ap");
            auto wg = u::make_var_ref("_s__wg");
            auto lmem = u::make_var_ref(LMEM_VAR);
            auto const ndim = it->second;

            auto i = static_cast<int>(fd->params.size() - 1);
            for (auto it = fd->params.rbegin(); it != fd->params.rend(); ++it, --i) {
                auto param = xcml::param_node::dyncast(*it);
                auto var = u::add_local_var(body, param->type, param->name);

                if (i < ndim) {
                    u::prepend_expr(
                        body, u::assign_expr(var, u::make_var_ref(fmt::format("_s__g{}", i))));
                } else if (i < 2 * ndim) {
                    auto const j = i - ndim;
                    u::prepend_expr(
                        body, u::assign_expr(var, u::make_var_ref(fmt::format("_s__l{}", j))));
                } else {
                    // TODO: __typeof__ is not a part of the standard
                    // ${it->type} ${it->name} = va_arg(ap, __typeof__(${it->name}));

                    auto typeof_ = u::make_call(u::make_func_addr("__typeof__"), {var});

                    u::prepend_expr(
                        body, u::assign_expr(var, u::make_call(u::make_func_addr("va_arg"),
                                                               {ap, typeof_})));
                }
            }

            ft->params.clear();
            fd->params.clear();
            u::add_param(ft, fd, get_or_create_voidptr_type(prg), lmem->name);
            u::add_param(ft, fd, get_or_create_voidptr_type(prg), wg->name);
            for (int i = 0; i < ndim; ++i) {
                u::add_param(ft, fd, "unsigned_long", fmt::format("_s__g{}", i));
            }
            for (int i = 0; i < ndim; ++i) {
                u::add_param(ft, fd, "unsigned_long", fmt::format("_s__l{}", i));
            }
            u::add_param(ft, fd, va_list_type, "_s__ap");
        }
    }
}

xcml::xcml_program_node_ptr lower_cpu(xcml::xcml_program_node_ptr const& prg,
                                      utils::target target) {
    math_transfomration_table math_funcs;

    math_funcs.includes.push_back("math.h");
    math_funcs.cos = forward_to_other_functions("float", "cosf", "double", "cos");
    math_funcs.exp = forward_to_other_functions("float", "expf", "double", "exp");
    math_funcs.sin = forward_to_other_functions("float", "sinf", "double", "sin");
    math_funcs.sqrt = forward_to_other_functions("float", "sqrtf", "double", "sqrt");
    math_funcs.tan = forward_to_other_functions("float", "tanf", "double", "tan");
    math_funcs.hypot = forward_to_other_functions("float", "hypotf", "double", "hypot");
    math_funcs.fdim = forward_to_other_functions("float", "fdimf", "double", "fdim");
    math_funcs.fmin = forward_to_other_functions("float", "fminf", "double", "fmin");
    math_funcs.fmax = forward_to_other_functions("float", "fmaxf", "double", "fmax");
    math_funcs.fabs = forward_to_other_functions("float", "fabsf", "double", "fabs");
    math_funcs.cbrt = forward_to_other_functions("float", "cbrtf", "double", "cbrt");

    auto new_prg = prg;
    new_prg = apply_visitor<add_function_loader_visitor>(new_prg, target);
    new_prg = apply_visitor<transform_kernel_wrapper>(new_prg);

    auto const cm = build_callmap(new_prg);
    auto const barrier_consumers = build_barrier_consumers(cm);
    auto const lmem_consumers = build_lmem_consumers(cm);

    std::unordered_map<std::string, int> ndr_kernels;
    new_prg = apply_visitor<transform_parallel_for>(new_prg, target, barrier_consumers, nullptr,
                                                    &ndr_kernels);

    new_prg = apply_visitor(make_transform_math_functions_visitor(math_funcs), new_prg);

    new_prg = apply_visitor<rewrite_args_visitor>(new_prg, lmem_consumers, barrier_consumers);

    replace_builtin_map_t replace_map;
    replace_map["__charm_sycl_local_memory_base"] = [](xcml::function_call_ptr const&) {
        return u::make_var_ref(LMEM_VAR);
    };

    replace_map["__charm_sycl_assume"] = [](xcml::function_call_ptr const& node) {
        node->function = u::make_func_addr("__builtin_assume");
        return node;
    };
    replace_map["__charm_sycl_group_barrier"] = [](xcml::function_call_ptr const& node) {
        node->arguments.push_back(u::make_var_ref(WG_VAR));
        return node;
    };
    new_prg = replace_builtin_function_calls(new_prg, replace_map);

    implement_builtin_map_t implement_map;
    bool use_fiber = false;
    implement_map["__charm_sycl_group_barrier"] =
        [&](xcml::xcml_program_node_ptr const& prg, xcml::function_decl_ptr const&,
            xcml::function_type_ptr const& ft, xcml::function_definition_ptr const& fd) {
            use_fiber = true;

            u::add_param(ft, get_or_create_voidptr_type(prg), WG_VAR);

            u::push_expr(fd->body, u::make_call(u::make_func_addr("__charm_sycl_fiber_barrier"),
                                                {u::make_var_ref(WG_VAR)}));
        };
    new_prg = implement_builtin_function_calls(new_prg, implement_map);

    if (use_fiber) {
        rewrite_kernel_args(new_prg, ndr_kernels);
    }

    return apply_visitors(new_prg, inline_functions, inline_functions_mandatory,
                          remove_unused_functions, fix_integral_casts, fix_address_of,
                          optimize_casts, optimize_compounds);
}

}  // namespace

xcml::xcml_program_node_ptr lower_cpu_c(xcml::xcml_program_node_ptr const& prg) {
    return lower_cpu(prg, utils::target::CPU_C);
}

xcml::xcml_program_node_ptr lower_cpu_openmp(xcml::xcml_program_node_ptr const& prg) {
    return lower_cpu(prg, utils::target::CPU_OPENMP);
}
