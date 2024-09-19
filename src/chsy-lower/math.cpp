#include "math.hpp"
#include <string>
#include <fmt/format.h>
#include <xcml_utils.hpp>

namespace u = xcml::utils;

void add_replace_math_func(replace_builtin_map_t& map, char const* name, char const* float_,
                           char const* double_) {
    map[fmt::format("__charm_sycl_{}_f", name)] =
        [float_ = std::string(float_)](xcml::function_call_ptr const& node) {
            node->function = xcml::utils::make_func_addr(float_);
            return node;
        };

    map[fmt::format("__charm_sycl_{}_d", name)] =
        [double_ = std::string(double_)](xcml::function_call_ptr const& node) {
            node->function = xcml::utils::make_func_addr(double_);
            return node;
        };
}

void add_common_replace_math_funcs(replace_builtin_map_t& map,
                                   implement_builtin_map_t& implement) {
    for (char const* fn : {
             "acos",  "acosh",    "asin",      "asinh",  "atan",  "atan2", "atanh", "cbrt",
             "ceil",  "cos",      "cosh",      "erf",    "erfc",  "exp",   "exp10", "exp2",
             "expm1", "fabs",     "floor",     "lgamma", "log10", "log1p", "log2",  "logb",
             "rint",  "round",    "sin",       "sinh",   "sqrt",  "tan",   "tanh",  "tgamma",
             "trunc", "copysign", "fdim",      "fmin",   "fmax",  "fmod",  "hypot", "nextafter",
             "pow",   "powr",     "remainder", "remquo",
         }) {
        auto const f = fmt::format("{}f", fn);
        add_replace_math_func(map, fn, f.c_str(), fn);
    }

    for (char const* fn : {"max", "min"}) {
        for (char const sig : {'c', 'h', 's', 't', 'i', 'j', 'l', 'm', 'x', 'y'}) {
            implement[fmt::format("__charm_sycl_{}_{}", fn, sig)] =
                [fn](xcml::xcml_program_node_ptr const&, xcml::function_decl_ptr const&,
                     xcml::function_type_ptr const&, xcml::function_definition_ptr const& fd) {
                    auto x = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(0))->name);
                    auto y = u::make_var_ref(xcml::param_node::dyncast(fd->params.at(1))->name);
                    auto cond = u::new_cond_expr();

                    cond->cond = u::log_lt_expr(x, y);
                    if (strcmp(fn, "max") == 0) {
                        cond->true_ = y;
                        cond->false_ = x;
                    } else {
                        cond->true_ = x;
                        cond->false_ = y;
                    }

                    u::push_stmt(fd->body, u::make_return(cond));
                };
        }
    }
}
