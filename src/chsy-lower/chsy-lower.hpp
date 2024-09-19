#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include <xcml_type.hpp>

xcml::xcml_program_node_ptr remove_unused_functions(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr fix_integral_casts(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr fix_address_of(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr optimize_compounds(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr optimize_casts(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr inline_functions(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr inline_functions_mandatory(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr array_as_vec(xcml::xcml_program_node_ptr);
xcml::xcml_program_node_ptr ext_vector_type(xcml::xcml_program_node_ptr);

xcml::xcml_program_node_ptr lower_cpu_c(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr lower_cpu_openmp(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr lower_nvidia_cuda(xcml::xcml_program_node_ptr const&);
xcml::xcml_program_node_ptr lower_amd_hip(xcml::xcml_program_node_ptr const&);

template <class V, class... Vs>
xcml::xcml_program_node_ptr apply_visitors(xcml::xcml_program_node_ptr const& node, V& v,
                                           Vs&&... vs) {
    if constexpr (sizeof...(Vs) == 0) {
        return v(node);
    } else {
        return apply_visitors(v(node), std::forward<Vs>(vs)...);
    }
}

using replace_builtin_map_t =
    std::unordered_map<std::string,
                       std::function<xcml::expr_ptr(xcml::function_call_ptr const&)>>;

xcml::xcml_program_node_ptr replace_builtin_function_calls(xcml::xcml_program_node_ptr prg,
                                                           replace_builtin_map_t const& map);

using implement_builtin_map_t = std::unordered_map<
    std::string,
    std::function<void(xcml::xcml_program_node_ptr const&, xcml::function_decl_ptr const&,
                       xcml::function_type_ptr const&, xcml::function_definition_ptr const&)>>;

xcml::xcml_program_node_ptr implement_builtin_function_calls(
    xcml::xcml_program_node_ptr prg, implement_builtin_map_t const& map);

template <class T, class... Args>
xcml::xcml_program_node_ptr apply_visitor(xcml::xcml_program_node_ptr const& prg,
                                          Args&&... args) {
    T vis(std::forward<Args>(args)...);
    return vis(prg);
}

template <class T>
xcml::xcml_program_node_ptr apply_visitor(std::unique_ptr<T> vis,
                                          xcml::xcml_program_node_ptr const& prg) {
    return (*vis)(prg);
}

struct vec_t {
    char const* sig;
    char const* scal;
    int veclen;
};

#define VEC(ty, len)           \
    vec_t {                    \
        "v" #len #ty, #ty, len \
    }

#define FLOAT_VECS_(ty) VEC(ty, 1), VEC(ty, 2), VEC(ty, 3), VEC(ty, 4), VEC(ty, 8), VEC(ty, 16)

inline const vec_t FLOAT_VECS[] = {FLOAT_VECS_(f), FLOAT_VECS_(d)};
