#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <xcml_type.hpp>

using function_generator = std::function<bool(std::string_view, xcml::function_type_ptr const&,
                                              xcml::function_definition_ptr const&)>;

struct math_transfomration_table {
    bool is_cuda = false;
    std::vector<std::string> includes;
    function_generator cos, exp, sin, sqrt, tan, hypot, fdim, min, fmin, max, fmax, clamp,
        length, fabs, cbrt;
};

struct transform_math_functions_visitor {
    virtual ~transform_math_functions_visitor() = default;

    virtual xcml::xcml_program_node_ptr apply(xcml::xcml_program_node_ptr const&) = 0;

    inline xcml::xcml_program_node_ptr operator()(xcml::xcml_program_node_ptr const& prg) {
        return apply(prg);
    }
};

std::unique_ptr<transform_math_functions_visitor> make_transform_math_functions_visitor(
    math_transfomration_table const&);

bool forward_to_other_function_impl(std::string_view func_name,
                                    xcml::function_definition_ptr const& fd);

struct forward_to_other_function {
    explicit forward_to_other_function(std::string_view func_name) : func_name_(func_name) {}

    bool operator()(std::string_view, xcml::function_type_ptr const&,
                    xcml::function_definition_ptr const& fd) const {
        return forward_to_other_function_impl(func_name_, fd);
    }

private:
    std::string func_name_;
};

struct forward_to_other_functions {
    forward_to_other_functions() = default;

    template <class... Args>
    explicit forward_to_other_functions(std::string const& type, std::string const& func_name,
                                        Args const&... args)
        : forward_to_other_functions(args...) {
        map_.insert_or_assign(type, func_name);
    }

    bool operator()(std::string_view type, xcml::function_type_ptr const&,
                    xcml::function_definition_ptr const& fd) const {
        auto it = map_.find(std::string(type));
        if (it == map_.end()) {
            return false;
        }
        return forward_to_other_function_impl(it->second, fd);
    }

private:
    std::unordered_map<std::string, std::string> map_;
};
