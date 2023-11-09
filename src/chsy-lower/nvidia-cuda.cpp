#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utils/target.hpp>
#include <xcml_recusive_visitor.hpp>
#include <xcml_utils.hpp>
#include "chsy-lower.hpp"
#include "math.hpp"

namespace {

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
            auto const is_memory = !!node->memories.count(i);

            if (is_memory) {
                auto const ptr = add_param(ft, fd, param->type, gen_var("ptr"));
                auto const range0 = add_param(ft, fd, rts_accessor_type(), gen_var("range"));
                auto const range = make_addr_of(range0);

                auto const& param2 = xcml::param_node::dyncast(node->params.at(i + 1));
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
            } else {
                add_param(ft, fd, param->type, param->name);
            }
        }

        fd->body->body.push_back(node->body);

        return fd;
    }
};

struct transform_parallel_invoke : xcml::recursive_visitor<transform_parallel_invoke> {
    xcml::node_ptr visit_parallel_invoke(xcml::parallel_invoke_ptr const& node,
                                         scope_ref scope) {
        using namespace xcml::utils;

        auto const ndim = node->dimensions.size();

        auto top = new_compound_stmt();
        auto body = top;
        std::vector<xcml::var_ref_ptr> iters;

        for (auto const& dim : node->dimensions) {
            iters.push_back(add_local_var(body, dim->type, dim->iter));
        }

        for (size_t dim = 0; dim < 3; ++dim) {
            if (ndim > dim) {
                auto idx = plus_expr(mul_expr(blockIdx(ndim, dim), blockDim(ndim, dim)),
                                     threadIdx(ndim, dim));
                auto off = node->dimensions.at(dim)->offset;
                auto expr = assign_expr(iters.at(dim), plus_expr(idx, off));

                push_expr(body, expr);
            }
        }

        for (size_t i = 0; i < ndim; ++i) {
            auto const if_ = new_if_stmt();
            auto const& dim = node->dimensions.at(i);

            if_->condition = log_lt_expr(iters.at(i), dim->size);

            body->body.push_back(if_);
            body = if_->then;
        }

        auto const call = make_call(xcml::func_addr::dyncast(node->function));

        call->arguments.insert(call->arguments.end(), iters.begin(), iters.end());

        for (auto const& arg : node->arguments) {
            auto const arg_type = get_type(compute_type(arg, scope.value()));

            if (xcml::struct_type::is_a(arg_type)) {
                call->arguments.push_back(make_addr_of(arg));
            } else {
                call->arguments.push_back(arg);
            }
        }

        push_expr(body, call);

        return top;
    }

    xcml::node_ptr visit_ndr_invoke(xcml::ndr_invoke_ptr const& node, scope_ref scope) {
        using namespace xcml::utils;

        auto top = new_compound_stmt();
        auto body = top;

        assert(node->group.size() == node->local.size());

        auto const ndim = node->group.size();
        std::vector<xcml::var_ref_ptr> iters;

        for (auto const& dim : node->group) {
            iters.push_back(add_local_var(body, dim->type, dim->iter));
        }
        for (size_t i = 0; i < ndim; i++) {
            push_expr(body, assign_expr(iters.at(i), blockIdx(ndim, i)));
        }
        for (auto const& dim : node->local) {
            iters.push_back(add_local_var(body, dim->type, dim->iter));
        }
        for (size_t i = 0; i < ndim; i++) {
            push_expr(body, assign_expr(iters.at(ndim + i), threadIdx(ndim, i)));
        }

        auto const call = make_call(xcml::func_addr::dyncast(node->function));

        call->arguments.insert(call->arguments.end(), iters.begin(), iters.end());

        for (auto const& arg : node->arguments) {
            auto const arg_type = get_type(compute_type(arg, scope.value()));

            if (xcml::struct_type::is_a(arg_type)) {
                call->arguments.push_back(make_addr_of(arg));
            } else {
                call->arguments.push_back(arg);
            }
        }

        push_expr(body, call);

        return top;
    }

private:
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

    xcml::type_ptr dim3_type() {
        for (auto const& type : root()->type_table) {
            if (auto bt = xcml::basic_type::dyncast(type)) {
                if (bt->name == "dim3" && !bt->is_const) {
                    return bt;
                }
            }
        }

        return create_basic_type("dim3", false, true);
    }
};

xcml::xcml_program_node_ptr do_lower(utils::target target,
                                     xcml::xcml_program_node_ptr const& prg) {
    math_transfomration_table math_funcs;

    if (is_cuda(target)) {
        math_funcs.includes.push_back("cuda_runtime.h");
    }
    math_funcs.cos = forward_to_other_function("std::cos");
    math_funcs.exp = forward_to_other_function("std::exp");
    math_funcs.sin = forward_to_other_function("std::sin");
    math_funcs.sqrt = forward_to_other_function("std::sqrt");
    math_funcs.tan = forward_to_other_function("std::tan");
    math_funcs.hypot = forward_to_other_function("std::hypot");
    math_funcs.fdim = forward_to_other_function("std::fdim");
    math_funcs.fmin = forward_to_other_function("std::fmin");
    math_funcs.fmax = forward_to_other_function("std::fmax");
    math_funcs.fabs = forward_to_other_function("std::abs");
    math_funcs.cbrt = forward_to_other_function("std::cbrt");

    transform_kernel_wrapper v1;
    transform_parallel_invoke v2;
    auto v3 = make_transform_math_functions_visitor(math_funcs);
    mark_as_device v4;
    implement_builtin_functions_visitor v5;

    return apply_visitors(prg, v1, v2, *v3, v4, v5, inline_functions,
                          inline_functions_mandatory, remove_unused_functions,
                          fix_integral_casts, fix_address_of, optimize_casts,
                          optimize_compounds);
}

}  // namespace

xcml::xcml_program_node_ptr lower_nvidia_cuda(xcml::xcml_program_node_ptr const& prg) {
    return do_lower(utils::target::NVIDIA_CUDA, prg);
}

xcml::xcml_program_node_ptr lower_amd_hip(xcml::xcml_program_node_ptr const& prg) {
    auto inc = xcml::new_cpp_include();
    inc->name = "hip/hip_runtime.h";
    prg->global_declarations.push_front(inc);

    return do_lower(utils::target::AMD_HIP, prg);
}
