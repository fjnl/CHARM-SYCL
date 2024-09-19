#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utils/target.hpp>
#include <xcml_recusive_visitor.hpp>
#include <xcml_utils.hpp>
#include "chsy-lower.hpp"
#include "math.hpp"

namespace {

namespace u = xcml::utils;

char const* CUDA_UTILS = R"(
#include <stdint.h>

[[maybe_unused]] inline __device__ bool __charm_sycl_is_leader_lane() {
    uint32_t v;
    asm volatile("mov.u32 %0, %laneid;" : "=r"(v));
    return v == 0;
}

template <class T>
[[maybe_unused]] inline __device__ T __charm_sycl_sum_in_warp(T local_val) {
#pragma unroll
    for (unsigned delta = warpSize / 2; delta > 0; delta /= 2) {
        local_val += __shfl_down_sync(0xffffffff, local_val, delta);
    }
    return local_val;
};
)";

char const* HIP_UTILS = R"(
#include <hip/device_functions.h>
#include <stdint.h>

[[maybe_unused]] inline __device__ bool __charm_sycl_is_leader_lane() {
    return __lane_id() == 0;
}

template <class T>
[[maybe_unused]] inline __device__ T __charm_sycl_sum_in_warp(T local_val) {
#pragma unroll
    for (unsigned delta = warpSize / 2; delta > 0; delta /= 2) {
        local_val += __shfl_down(local_val, delta);
        __syncthreads();
    }
    return local_val;
};
)";

inline xcml::cuda_attribute_ptr cuda_device() {
    auto device = xcml::new_cuda_attribute();
    device->value = "__device__";
    return device;
}

inline xcml::cuda_attribute_ptr cuda_global() {
    auto global = xcml::new_cuda_attribute();
    global->value = "__global__";
    return global;
}

inline xcml::cuda_attribute_ptr cuda_shared() {
    auto shared = xcml::new_cuda_attribute();
    shared->value = "__shared__";
    return shared;
}

static xcml::expr_ptr dim3_ref(xcml::expr_ptr const& var, size_t ndim, size_t dim) {
    using namespace xcml::utils;

    switch (ndim) {
        case 1:
            return make_member_ref(var, "x");

        case 2:
            switch (dim) {
                case 0:
                    return make_member_ref(var, "y");
                default:
                    return make_member_ref(var, "x");
            }

        default:
            switch (dim) {
                case 0:
                    return make_member_ref(var, "z");
                case 1:
                    return make_member_ref(var, "y");
                default:
                    return make_member_ref(var, "x");
            }
    }
}

static xcml::expr_ptr gridDim(size_t ndim, size_t dim) {
    using namespace xcml::utils;

    return dim3_ref(make_var_addr("gridDim"), ndim, dim);
}

static xcml::expr_ptr blockIdx(size_t ndim, size_t dim) {
    using namespace xcml::utils;

    return dim3_ref(make_var_addr("blockIdx"), ndim, dim);
}

static xcml::expr_ptr blockDim(size_t ndim, size_t dim) {
    using namespace xcml::utils;

    return dim3_ref(make_var_addr("blockDim"), ndim, dim);
}

static xcml::expr_ptr threadIdx(size_t ndim, size_t dim) {
    using namespace xcml::utils;

    return dim3_ref(make_var_addr("threadIdx"), ndim, dim);
}

struct mark_as_device final : xcml::recursive_visitor<mark_as_device> {
    using parent = recursive_visitor<mark_as_device>;

    xcml::node_ptr visit_function_type(xcml::function_type_ptr const& node, scope_ref scope) {
        if (node->cuda_attrs.empty()) {
            node->cuda_attrs.push_back(cuda_device());
        }
        return parent::visit_function_type(node, scope);
    }
};

struct implement_builtin_functions_visitor final
    : xcml::recursive_visitor<implement_builtin_functions_visitor> {
    using parent = xcml::recursive_visitor<implement_builtin_functions_visitor>;

    implement_builtin_functions_visitor() {
        init();
    }

    xcml::node_ptr visit_function_decl(xcml::function_decl_ptr const& node, scope_ref scope) {
        if (node->name.find("__charm_sycl_") == 0) {
            auto const& ft = lookup(scope, node->name).function_type();
            do_implement(node->name, node, ft);
        }
        return parent::visit_function_decl(node, scope);
    }

private:
    void do_implement(std::string const& name, xcml::function_decl_ptr const& decl,
                      xcml::function_type_ptr const& ft) {
        auto it = handlers_.find(name);
        if (it != handlers_.end()) {
            auto fd = xcml::utils::make_fd(root(), ft, name, true);

            it->second(decl, ft, fd);

            root()->global_declarations.push_back(fd);

            handlers_.erase(it);
        }
    }

    void init() {
        namespace u = xcml::utils;

        handlers_["__charm_sycl_group_barrier"] = [](xcml::function_decl_ptr const&,
                                                     xcml::function_type_ptr const&,
                                                     xcml::function_definition_ptr const& fd) {
            u::push_expr(fd->body, u::make_call(u::make_func_addr("__syncthreads")));
        };

        handlers_["__charm_sycl_local_memory_base"] =
            [](xcml::function_decl_ptr const&, xcml::function_type_ptr const&,
               xcml::function_definition_ptr const& fd) {
                auto sym = u::new_symbol_id();
                sym->sclass = xcml::storage_class::extern_;
                sym->name = "__charm_sycl_smem_base[]";
                sym->type = "char";
                sym->cudaAttributes.push_back(cuda_shared());

                auto decl = u::new_var_decl();
                decl->name = sym->name;

                fd->body->symbols.push_back(sym);
                fd->body->declarations.push_back(decl);
                u::push_stmt(fd->body,
                             u::make_return(u::make_var_ref("__charm_sycl_smem_base")));
            };

        handlers_["__charm_sycl_assume"] = [](xcml::function_decl_ptr const&,
                                              xcml::function_type_ptr const&,
                                              xcml::function_definition_ptr const& fd) {
            auto param = xcml::param_node::dyncast(fd->params.at(0));
            auto cond = u::make_var_ref(param->name);

            u::push_expr(fd->body, u::make_call(u::make_func_addr("__builtin_assume"), {cond}));
        };

        handlers_["__charm_sycl_is_reduce_leader_1"] =
            [](xcml::function_decl_ptr const&, xcml::function_type_ptr const&,
               xcml::function_definition_ptr const& fd) {
                /*
                 * return threadIdx.x == 0;
                 */
                u::push_stmt(fd->body, u::make_return(u::log_eq_expr(
                                           u::make_var_ref("threadIdx.x"), u::lit(0))));
            };

        handlers_["__charm_sycl_is_reduce_leader_2"] =
            [](xcml::function_decl_ptr const&, xcml::function_type_ptr const&,
               xcml::function_definition_ptr const& fd) {
                /*
                 * return threadIdx.x == 0 && threadIdx.y == 0;
                 */
                u::push_stmt(fd->body,
                             u ::make_return(u::log_and_expr(
                                 u::log_eq_expr(u::make_var_ref("threadIdx.x"), u::lit(0)),
                                 u::log_eq_expr(u::make_var_ref("threadIdx.y"), u::lit(0)))));
            };

        handlers_["__charm_sycl_is_reduce_leader_3"] =
            [](xcml::function_decl_ptr const&, xcml::function_type_ptr const&,
               xcml::function_definition_ptr const& fd) {
                /*
                 * return threadIdx.x == 0 && threadIdx.y == 0 && threadIdx.z == 0;
                 */
                u::push_stmt(fd->body,
                             u::make_return(u::log_and_expr(
                                 u::log_and_expr(
                                     u::log_eq_expr(u::make_var_ref("threadIdx.x"), u::lit(0)),
                                     u::log_eq_expr(u::make_var_ref("threadIdx.y"), u::lit(0))),
                                 u::log_eq_expr(u::make_var_ref("threadIdx.z"), u::lit(0)))));
            };

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

            handlers_[init_fn] = [](xcml::function_decl_ptr const&,
                                    xcml::function_type_ptr const&,
                                    xcml::function_definition_ptr const& fd) {
                auto val = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(0))->name);

                /*
                 * *val = 0;
                 */
                u::push_expr(fd->body, u::assign_expr(u::make_deref(val), u::lit(0)));
            };

            handlers_[comb_fn] = [](xcml::function_decl_ptr const&,
                                    xcml::function_type_ptr const&,
                                    xcml::function_definition_ptr const& fd) {
                /*
                 * *val += partial;
                 */
                auto val = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(0))->name);
                auto partial =
                    u::make_var_ref(xcml::param_node::dyncast(fd->params.at(1))->name);

                u::push_expr(fd->body, u::asg_plus_expr(u::make_deref(val), partial));
            };

            handlers_[fini_fn] = [](xcml::function_decl_ptr const&,
                                    xcml::function_type_ptr const&,
                                    xcml::function_definition_ptr const& fd) {
                auto g_ptr = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(0))->name);
                auto param1 = xcml::param_node::dyncast(fd->params.at(1));
                auto val = u::make_var_ref(param1->name);
                auto value_type = param1->type;
                auto is_leader =
                    u::make_var_ref(xcml::param_node::dyncast(fd->params.at(2))->name);

                /*
                 * __shared__ ${value_type} red;
                 */
                auto red = u::add_local_var(fd->body, value_type, "_s__red");
                auto red_ptr = u::make_addr_of(red);
                fd->body->symbols.back()->cudaAttributes.push_back(cuda_shared());

                auto red_init = u::new_if_stmt();
                red_init->condition = is_leader;
                u::push_expr(red_init->then, u::assign_expr(red, u::lit(0)));

                /*
                 * if (is_leader) {
                 *     red = 0;
                 * }
                 */
                u::push_stmt(fd->body, red_init);
                /*
                 * __syncthreads();
                 */
                u::push_expr(fd->body, u::make_call(u::make_func_addr("__syncthreads")));

                /*
                 * val = __charm_sycl_sum_in_warp(val);
                 */
                u::push_expr(
                    fd->body,
                    u::assign_expr(
                        val,
                        u::make_call(u::make_func_addr("__charm_sycl_sum_in_warp"), {val})));

                auto if_is_leader_lane = u::new_if_stmt();
                if_is_leader_lane->condition =
                    u::make_call(u::make_func_addr("__charm_sycl_is_leader_lane"), {});

                u::push_expr(if_is_leader_lane->then,
                             u::make_call(u::make_func_addr("atomicAdd"), {red_ptr, val}));

                /*
                 * if (__charm_sycl_is_leader_lane()) {
                 *     atomicAdd(&red, val);
                 * }
                 */
                u::push_stmt(fd->body, if_is_leader_lane);
                /*
                 * __syncthreads();
                 */
                u::push_expr(fd->body, u::make_call(u::make_func_addr("__syncthreads")));

                auto if_leader = u::new_if_stmt();
                if_leader->condition = is_leader;

                u::push_expr(if_leader->then,
                             u::make_call(u::make_func_addr("atomicAdd"), {g_ptr, red}));
                /*
                 * if (is_leader) {
                 *     atomicAdd(g_ptr, red);
                 * }
                 */
                u::push_stmt(fd->body, if_leader);
            };
        }
    }

    std::unordered_map<std::string, std::function<void(xcml::function_decl_ptr const&,
                                                       xcml::function_type_ptr const&,
                                                       xcml::function_definition_ptr const&)>>
        handlers_;
};

struct transform_kernel_wrapper final : xcml::recursive_visitor<transform_kernel_wrapper> {
    xcml::node_ptr visit_kernel_wrapper_decl(xcml::kernel_wrapper_decl_ptr const& node,
                                             scope_ref) {
        using namespace xcml::utils;

        auto ft = create_function_type();
        ft->return_type = "void";
        ft->cuda_attrs.push_back(cuda_global());

        fdecl_opts opts;
        opts.is_static = false;
        opts.extern_c = true;

        add_fdecl(root(), ft, node->name, opts);
        auto const fd = make_fd(root(), ft, node->name, true);

        for (size_t i = 0; i < node->params.size(); i++) {
            auto param = xcml::param_node::dyncast(node->params.at(i));

            add_param(ft, fd, param->type, param->name);
        }

        fd->body->body.push_back(node->body);

        return fd;
    }
};

void add_common_replacements(replace_builtin_map_t& replace) {
    replace["__charm_sycl_kernel"] = [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
        return nullptr;
    };

    for (int dim = 0; dim < 3; ++dim) {
        replace[fmt::format("__charm_sycl_parallel_iter{}_begin", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return u::plus_expr(u::mul_expr(blockIdx(3, dim), blockDim(3, dim)),
                                threadIdx(3, dim));
        };

        replace[fmt::format("__charm_sycl_parallel_iter{}_cond", dim + 1)] =
            [=](xcml::function_call_ptr const& call) -> xcml::expr_ptr {
            return u::log_lt_expr(call->arguments.at(0), call->arguments.at(1));
        };

        replace[fmt::format("__charm_sycl_parallel_iter{}_step", dim + 1)] =
            [=](xcml::function_call_ptr const& call) -> xcml::expr_ptr {
            return u::plus_expr(call->arguments.at(0),
                                u::mul_expr(gridDim(3, dim), blockDim(3, dim)));
        };
    }

    for (int dim = 0; dim < 3; ++dim) {
        replace[fmt::format("__charm_sycl_group_range{}", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return gridDim(3, dim);
        };

        replace[fmt::format("__charm_sycl_group_id{}", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return blockIdx(3, dim);
        };

        replace[fmt::format("__charm_sycl_local_range{}", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return blockDim(3, dim);
        };

        replace[fmt::format("__charm_sycl_local_id{}", dim + 1)] =
            [=](xcml::function_call_ptr const&) -> xcml::expr_ptr {
            return threadIdx(3, dim);
        };
    }
}

xcml::xcml_program_node_ptr do_lower(utils::target target, xcml::xcml_program_node_ptr prg) {
    auto utils = xcml::utils::new_code();

    if (is_hip(target)) {
        prg = ext_vector_type(prg);
        utils->value = HIP_UTILS;
    } else {
        prg = array_as_vec(prg);
        utils->value = CUDA_UTILS;
    }
    prg->preamble.push_back(utils);

    prg = apply_visitor<transform_kernel_wrapper>(prg);

    replace_builtin_map_t replace_map;
    implement_builtin_map_t implement_map;
    add_common_replacements(replace_map);
    add_common_replace_math_funcs(replace_map, implement_map);
    prg = replace_builtin_function_calls(prg, replace_map);
    prg = implement_builtin_function_calls(prg, implement_map);

    prg = apply_visitor<mark_as_device>(prg);

    prg = apply_visitor<implement_builtin_functions_visitor>(prg);

    return apply_visitors(prg, inline_functions, inline_functions_mandatory,
                          remove_unused_functions, fix_integral_casts, fix_address_of,
                          optimize_casts, optimize_compounds);
}

}  // namespace

xcml::xcml_program_node_ptr lower_nvidia_cuda(xcml::xcml_program_node_ptr const& prg) {
    auto inc = xcml::new_cpp_include();
    inc->name = "cuda_runtime.h";
    prg->preamble.push_front(inc);

    return do_lower(utils::target::NVIDIA_CUDA, prg);
}

xcml::xcml_program_node_ptr lower_amd_hip(xcml::xcml_program_node_ptr const& prg) {
    auto inc = xcml::new_cpp_include();
    inc->name = "hip/hip_runtime.h";
    prg->preamble.push_front(inc);

    return do_lower(utils::target::AMD_HIP, prg);
}
