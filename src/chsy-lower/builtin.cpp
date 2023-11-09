#include <xcml.hpp>
#include <xcml_recusive_visitor.hpp>
#include "chsy-lower.hpp"

namespace u = xcml::utils;

struct replace_builtin_function_calls_visitor final
    : xcml::recursive_visitor<replace_builtin_function_calls_visitor> {
    using parent = xcml::recursive_visitor<replace_builtin_function_calls_visitor>;

    explicit replace_builtin_function_calls_visitor(replace_builtin_map_t const& map)
        : map_(map) {}

    xcml::node_ptr visit_function_call(xcml::function_call_ptr const& node, scope_ref scope) {
        auto fa = xcml::func_addr::dyncast(node->function);
        assert(fa);

        if (auto it = map_.find(fa->name); it != map_.end()) {
            return it->second(node);
        }

        return parent::visit_function_call(node, scope);
    }

private:
    replace_builtin_map_t const& map_;
};

xcml::xcml_program_node_ptr replace_builtin_function_calls(xcml::xcml_program_node_ptr prg,
                                                           replace_builtin_map_t const& map) {
    return apply_visitor<replace_builtin_function_calls_visitor>(prg, map);
}

struct implement_builtin_functions_visitor final
    : xcml::recursive_visitor<implement_builtin_functions_visitor> {
    using parent = xcml::recursive_visitor<implement_builtin_functions_visitor>;

    explicit implement_builtin_functions_visitor(implement_builtin_map_t const& map)
        : map_(map) {}

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
        auto it = map_.find(name);
        if (it != map_.end()) {
            auto fd = u::make_fd(root(), ft, name, true);

            it->second(root(), decl, ft, fd);

            root()->global_declarations.push_back(fd);

            map_.erase(it);
        }
    }

    implement_builtin_map_t map_;
};

xcml::xcml_program_node_ptr implement_builtin_function_calls(
    xcml::xcml_program_node_ptr prg, implement_builtin_map_t const& map) {
    return apply_visitor<implement_builtin_functions_visitor>(prg, map);
}
