#include "math.hpp"
#include <unordered_set>
#include <fmt/format.h>
#include <xcml.hpp>
#include <xcml_recusive_visitor.hpp>

namespace {

struct transform_math_functions_visitor_impl final
    : transform_math_functions_visitor,
      xcml::recursive_visitor<transform_math_functions_visitor_impl> {
    explicit transform_math_functions_visitor_impl(math_transfomration_table const& t)
        : t_(t) {}

    xcml::xcml_program_node_ptr apply(xcml::xcml_program_node_ptr const& prg) override {
        return (*static_cast<xcml::recursive_visitor<transform_math_functions_visitor_impl>*>(
            this))(prg);
    }

    xcml::node_ptr visit_runtime_func_decl(xcml::runtime_func_decl_ptr const& node,
                                           scope_ref scope) {
        using namespace xcml::utils;

        if (first_) {
            for (auto const& filename : t_.includes) {
                auto const inc = xcml::new_cpp_include();
                inc->name = filename;
                root()->global_declarations.push_back(inc);
            }

            first_ = false;
        }

        auto const& ft = lookup(scope, node->name).function_type();
        auto const& kind = node->func_kind;
        auto const fd = make_fd(root(), ft, node->name, true);

        fdecl_opts opts;
        opts.is_force_inline = true;
        opts.no_add_sym = true;
        add_fdecl(root(), ft, node->name, opts);

        if (t_.is_cuda) {
            auto host = xcml::new_cuda_attribute();
            host->value = "__host__";
            auto device = xcml::new_cuda_attribute();
            device->value = "__device__";

            ft->cuda_attrs.push_back(host);
            ft->cuda_attrs.push_back(device);
        }

#define CALL_GEN(name)                          \
    if (kind == #name && t_.name) {             \
        if (t_.name(ft->return_type, ft, fd)) { \
            return fd;                          \
        }                                       \
    }

        CALL_GEN(cos)
        CALL_GEN(exp)
        CALL_GEN(sin)
        CALL_GEN(sqrt)
        CALL_GEN(tan)
        CALL_GEN(hypot)
        CALL_GEN(fdim)
        CALL_GEN(min)
        CALL_GEN(fmin)
        CALL_GEN(max)
        CALL_GEN(fmax)
        CALL_GEN(clamp)
        CALL_GEN(length)
        CALL_GEN(fabs)
        CALL_GEN(cbrt)

#undef CALL_GEN

        throw std::runtime_error(
            fmt::format("Runtime function `{} {}` is not supported\n", ft->return_type, kind));
    }

private:
    math_transfomration_table const& t_;
    bool first_ = true;
};

}  // namespace

std::unique_ptr<transform_math_functions_visitor> make_transform_math_functions_visitor(
    math_transfomration_table const& t) {
    return std::make_unique<transform_math_functions_visitor_impl>(t);
}

bool forward_to_other_function_impl(std::string_view func_name,
                                    xcml::function_definition_ptr const& fd) {
    using namespace xcml::utils;

    auto const fa = make_func_addr(func_name);

    std::vector<xcml::expr_ptr> args;
    for (auto const& p : fd->params) {
        auto param = xcml::param_node::dyncast(p);

        args.push_back(make_var_ref(param->name));
    }

    auto const call = make_call(fa, args.begin(), args.end());
    push_stmt(fd->body, make_return(call));
    return true;
}
