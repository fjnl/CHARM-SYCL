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
        call->arguments.push_back(lit(node->is_ndr));

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
            auto int_type = get_basic_type("int");

            add_param(ft, char_const_ptr, "name");
            add_param(ft, unsigned_long, "name_hash");
            add_param(ft, char_const_ptr, "kind");
            add_param(ft, unsigned_long, "kind_hash");
            add_param(ft, void_ptr, "f");
            add_param(ft, int_type, "is_ndr");

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

            auto const is_struct = xcml::struct_type::is_a(type);
            auto const is_array = xcml::array_type::is_a(type);

            if (is_struct) {
                add_local_var(fd->body, type, param->name, make_deref(value));
            } else if (is_array) {
                auto var = add_local_var(fd->body, type, param->name);
                auto arg = make_array_ref(args, lit(i));

                auto call =
                    u::make_call(u::make_func_addr("memcpy"),
                                 {var, arg, u::make_call(u::make_func_addr("sizeof"), {var})});

                u::push_expr(fd->body, call);
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

void add_common_replacements(replace_builtin_map_t& replace) {
    replace["__charm_sycl_kernel"] = [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
        return nullptr;
    };

    for (int dim = 1; dim <= 3; ++dim) {
        replace[fmt::format("__charm_sycl_parallel_iter{}_begin", dim)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return u::lit(0);
        };

        replace[fmt::format("__charm_sycl_parallel_iter{}_cond", dim)] =
            [=](xcml::function_call_ptr const& call) -> xcml::expr_ptr {
            return u::log_lt_expr(call->arguments.at(0), call->arguments.at(1));
        };

        replace[fmt::format("__charm_sycl_parallel_iter{}_step", dim)] =
            [=](xcml::function_call_ptr const& call) -> xcml::expr_ptr {
            return u::plus_expr(call->arguments.at(0), u::lit(1));
        };
    }

    for (int dim = 0; dim < 3; ++dim) {
        replace[fmt::format("__charm_sycl_group_range{}", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return u::make_call(
                u::make_func_addr(fmt::format("__charm_sycl_fiber_group_range{}", dim + 1)),
                {});
        };

        replace[fmt::format("__charm_sycl_group_id{}", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return u::make_call(
                u::make_func_addr(fmt::format("__charm_sycl_fiber_group_id{}", dim + 1)), {});
        };

        replace[fmt::format("__charm_sycl_local_range{}", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return u::make_call(
                u::make_func_addr(fmt::format("__charm_sycl_fiber_local_range{}", dim + 1)),
                {});
        };

        replace[fmt::format("__charm_sycl_local_id{}", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return u::make_call(
                u::make_func_addr(fmt::format("__charm_sycl_fiber_local_id{}", dim + 1)), {});
        };
    }
}

void add_reduction_funcs(implement_builtin_map_t& implement_map) {
    for (auto dim : {1, 2, 3}) {
        implement_map[fmt::format("__charm_sycl_is_reduce_leader_{}", dim)] =
            [](xcml::xcml_program_node_ptr const&, xcml::function_decl_ptr const&,
               xcml::function_type_ptr const&, xcml::function_definition_ptr const& fd) {
                u::push_stmt(fd->body, u::make_return(u::lit(1)));
            };
    }

    struct type_pair {
        char const* t;
        char const* ty;
    };

    for (type_pair const& x : (type_pair[]){
             {"f", "float"},
             {"d", "double"},
         }) {
        auto const init_fn = fmt::format("__charm_sycl_reduce_initialize_{}", x.t);
        auto const comb_fn = fmt::format("__charm_sycl_reduce_combine_{}", x.t);
        auto const fini_fn = fmt::format("__charm_sycl_reduce_finalize_{}", x.t);

        implement_map[init_fn] =
            [](xcml::xcml_program_node_ptr const&, xcml::function_decl_ptr const&,
               xcml::function_type_ptr const&, xcml::function_definition_ptr const& fd) {
                auto val = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(0))->name);

                /*
                 * *val = 0;
                 */
                u::push_expr(fd->body, u::assign_expr(u::make_deref(val), u::lit(0)));
            };

        implement_map[comb_fn] = [](xcml::xcml_program_node_ptr const&,
                                    xcml::function_decl_ptr const&,
                                    xcml::function_type_ptr const&,
                                    xcml::function_definition_ptr const& fd) {
            auto val = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(0))->name);
            auto partial = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(1))->name);

            /*
             * *val += partial;
             */
            u::push_expr(fd->body, u::asg_plus_expr(u::make_deref(val), partial));
        };

        implement_map[fini_fn] =
            [](xcml::xcml_program_node_ptr const&, xcml::function_decl_ptr const&,
               xcml::function_type_ptr const&, xcml::function_definition_ptr const& fd) {
                auto g_ptr = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(0))->name);
                auto val = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(1))->name);

                /*
                 * __atomic_add_fetch(g_ptr, val, __ATOMIC_SEQ_CST);
                 */
                u::push_expr(fd->body,
                             u::make_call(u::make_func_addr("__atomic_fetch_add"),
                                          {g_ptr, val, u::make_var_ref("__ATOMIC_SEQ_CST")}));
            };
    }
}

struct add_fiber_funcs final : xcml::recursive_visitor<add_fiber_funcs> {
    xcml::node_ptr visit_xcml_program_node(xcml::xcml_program_node_ptr const& node, scope_ref) {
        add_barrier();
        add_memory();
        add_group_ranges();
        add_group_ids();
        add_local_ranges();
        add_local_ids();
        return node;
    }

    void add_barrier() {
        auto ft = create_function_type();

        ft->return_type = "void";

        u::fdecl_opts opts;
        opts.extern_c = true;
        u::add_fdecl(root(), ft, "__charm_sycl_fiber_barrier", opts);
    }

    void add_memory() {
        auto void_t = get_basic_type("void");
        auto void_ptr_t = get_pointer_type(void_t);
        auto ft = create_function_type();

        ft->return_type = void_ptr_t->type;

        u::fdecl_opts opts;
        opts.extern_c = true;
        u::add_fdecl(root(), ft, "__charm_sycl_fiber_memory", opts);
    }

    void add_group_ranges() {
        for (int dim = 0; dim < 3; ++dim) {
            auto ft = create_function_type();

            ft->return_type = "unsigned_long";

            u::fdecl_opts opts;
            opts.extern_c = true;
            u::add_fdecl(root(), ft, fmt::format("__charm_sycl_fiber_group_range{}", dim + 1),
                         opts);
        }
    }

    void add_group_ids() {
        for (int dim = 0; dim < 3; ++dim) {
            auto ft = create_function_type();

            ft->return_type = "unsigned_long";

            u::fdecl_opts opts;
            opts.extern_c = true;
            u::add_fdecl(root(), ft, fmt::format("__charm_sycl_fiber_group_id{}", dim + 1),
                         opts);
        }
    }

    void add_local_ranges() {
        for (int dim = 0; dim < 3; ++dim) {
            auto ft = create_function_type();

            ft->return_type = "unsigned_long";

            u::fdecl_opts opts;
            opts.extern_c = true;
            u::add_fdecl(root(), ft, fmt::format("__charm_sycl_fiber_local_range{}", dim + 1),
                         opts);
        }
    }

    void add_local_ids() {
        for (int dim = 0; dim < 3; ++dim) {
            auto ft = create_function_type();

            ft->return_type = "unsigned_long";

            u::fdecl_opts opts;
            opts.extern_c = true;
            u::add_fdecl(root(), ft, fmt::format("__charm_sycl_fiber_local_id{}", dim + 1),
                         opts);
        }
    }
};

xcml::xcml_program_node_ptr lower_cpu(xcml::xcml_program_node_ptr prg, utils::target target) {
    auto math_h = xcml::new_cpp_include();
    math_h->name = "math.h";
    prg->preamble.push_front(math_h);

    auto string_h = xcml::new_cpp_include();
    string_h->name = "string.h";
    prg->preamble.push_front(string_h);

    prg = array_as_vec(prg);

    prg = apply_visitor<add_function_loader_visitor>(prg, target);
    prg = apply_visitor<transform_kernel_wrapper>(prg);

    replace_builtin_map_t replace_map;
    implement_builtin_map_t implement_map;

    add_common_replacements(replace_map);

    replace_map["__charm_sycl_local_memory_base"] = [&](xcml::function_call_ptr const&) {
        return u::make_call(u::make_func_addr("__charm_sycl_fiber_memory"), {});
    };

    replace_map["__charm_sycl_assume"] = [](xcml::function_call_ptr const& node) {
        node->function = u::make_func_addr("__builtin_assume");
        return node;
    };

    implement_map["__charm_sycl_group_barrier"] =
        [&](xcml::xcml_program_node_ptr const& prg, xcml::function_decl_ptr const&,
            xcml::function_type_ptr const& ft, xcml::function_definition_ptr const& fd) {
            (void)prg;
            (void)ft;
            (void)fd;
            u::push_expr(fd->body,
                         u::make_call(u::make_func_addr("__charm_sycl_fiber_barrier"), {}));
        };

    add_reduction_funcs(implement_map);
    add_common_replace_math_funcs(replace_map, implement_map);
    prg = replace_builtin_function_calls(prg, replace_map);
    prg = implement_builtin_function_calls(prg, implement_map);

    prg = apply_visitor<add_fiber_funcs>(prg);

    return apply_visitors(prg, inline_functions, inline_functions_mandatory,
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
