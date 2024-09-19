#include <cstdlib>
#include <unordered_set>
#include <fmt/format.h>
#include <xcml.hpp>
#include <xcml_recusive_visitor.hpp>
#include "chsy-lower.hpp"

namespace {

using namespace xcml::utils;

struct inliner : xcml::visitor<inliner, xcml::node_ptr> {
    using arg_map_t = std::unordered_map<std::string, xcml::expr_ptr>;

#define EXPR_2(expr)                                    \
    ({                                                  \
        if (expr) {                                     \
            auto _x = as_expr(visit(expr, funcs, map)); \
            expr = _x;                                  \
        }                                               \
    })

#define EXPR(var) EXPR_2(node->var)

#define COMPOUND(var)                                 \
    ({                                                \
        if (node->var) {                              \
            auto _x = (*this)(node->var, funcs, map); \
            node->var = _x;                           \
        }                                             \
    })

    explicit inliner(bool ignore_failed) : ignore_failed_(ignore_failed) {}

    void operator()(xcml::xcml_program_node_ptr prg, xcml::function_definition_ptr fd,
                    std::unordered_set<std::string> const& funcs) {
        prg_ = prg;
        visit(fd->body, funcs, nullptr);
    }

    xcml::compound_stmt_ptr operator()(xcml::compound_stmt_ptr node,
                                       std::unordered_set<std::string> const& funcs,
                                       arg_map_t const* map) {
        for (auto& decl : node->declarations) {
            decl = xcml::decl_node::dyncast(visit(decl, funcs, map));
        }

        for (auto& stmt : node->body) {
            stmt = as_stmt(visit(stmt, funcs, map));
        }

        return node;
    }

    xcml::node_ptr visit_var_decl(xcml::var_decl_ptr const& node,
                                  std::unordered_set<std::string> const&, arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_unary_op(std::shared_ptr<xcml::unary_op> node,
                                  std::unordered_set<std::string> const& funcs,
                                  arg_map_t const* map) {
        EXPR(expr);
        return node;
    }

    xcml::node_ptr visit_binary_op(std::shared_ptr<xcml::binary_op> node,
                                   std::unordered_set<std::string> const& funcs,
                                   arg_map_t const* map) {
        EXPR(lhs);
        EXPR(rhs);
        return node;
    }

    xcml::node_ptr visit_symbol_id(xcml::symbol_id_ptr node,
                                   std::unordered_set<std::string> const&, arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_compound_stmt(xcml::compound_stmt_ptr node,
                                       std::unordered_set<std::string> const& funcs,
                                       arg_map_t const* map) {
        return (*this)(node, funcs, map);
    }

    xcml::node_ptr visit_for_stmt(xcml::for_stmt_ptr node,
                                  std::unordered_set<std::string> const& funcs,
                                  arg_map_t const* map) {
        EXPR(init);
        EXPR(condition);
        EXPR(iter);
        COMPOUND(body);
        return node;
    }

    xcml::node_ptr visit_if_stmt(xcml::if_stmt_ptr node,
                                 std::unordered_set<std::string> const& funcs,
                                 arg_map_t const* map) {
        EXPR(condition);
        COMPOUND(then);
        COMPOUND(else_);
        return node;
    }

    xcml::node_ptr visit_return_stmt(xcml::return_stmt_ptr node,
                                     std::unordered_set<std::string> const& funcs,
                                     arg_map_t const* map) {
        EXPR(value);
        return node;
    }

    xcml::node_ptr visit_pragma(xcml::pragma_ptr node, std::unordered_set<std::string> const&,
                                arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_expr_stmt(xcml::expr_stmt_ptr node,
                                   std::unordered_set<std::string> const& funcs,
                                   arg_map_t const* map) {
        if (auto const call = xcml::function_call::dyncast(node->expr)) {
            auto const c = new_compound_stmt();

            if (auto const new_call = visit_function_call(call, funcs, map, c)) {
                // The call is not inlined
                node->expr = xcml::expr_node::dyncast(new_call);
                return node;
            }

            // The call is inlined inside `c`
            return c;
        }

        EXPR(expr);
        return node;
    }

    // xcml::node_ptr visit_parallel_invoke(xcml::parallel_invoke_ptr node,
    //                                      std::unordered_set<std::string> const& funcs,
    //                                      arg_map_t const* map) {
    //     for (auto const& dim : node->dimensions) {
    //         EXPR_2(dim->offset);
    //         EXPR_2(dim->size);
    //     }
    //     EXPR(function);
    //     for (auto& arg : node->arguments) {
    //         EXPR_2(arg);
    //     }
    //     return node;
    // }

    xcml::node_ptr visit_var_ref(xcml::var_ref_ptr node, std::unordered_set<std::string> const&,
                                 arg_map_t const* map) {
        if (map) {
            auto it = map->find(node->name);

            if (it != map->end()) {
                return xcml::copy_expr(prg_, it->second);
            }
        }

        return node;
    }

    xcml::node_ptr visit_member_ref(xcml::member_ref_ptr node,
                                    std::unordered_set<std::string> const& funcs,
                                    arg_map_t const* map) {
        EXPR(value);
        return node;
    }

    xcml::node_ptr visit_func_addr(xcml::func_addr_ptr node,
                                   std::unordered_set<std::string> const&, arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_var_addr(xcml::var_addr_ptr node,
                                  std::unordered_set<std::string> const&, arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_array_addr(xcml::array_addr_ptr node,
                                    std::unordered_set<std::string> const&, arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_member_addr(xcml::member_addr_ptr node,
                                     std::unordered_set<std::string> const& funcs,
                                     arg_map_t const* map) {
        EXPR(value);
        return node;
    }

    xcml::node_ptr visit_pointer_ref(xcml::pointer_ref_ptr node,
                                     std::unordered_set<std::string> const& funcs,
                                     arg_map_t const* map) {
        EXPR(expr);
        return node;
    }

    xcml::node_ptr visit_array_ref(xcml::array_ref_ptr node,
                                   std::unordered_set<std::string> const& funcs,
                                   arg_map_t const* map) {
        EXPR(array);

        for (size_t i = 0, n = node->index.size(); i < n; i++) {
            auto arg = node->index.at(i);
            arg = as_expr(visit(arg, funcs, map));
            node->index.at(i) = arg;
        }

        return node;
    }

    xcml::node_ptr visit_member_array_ref(xcml::member_array_ref_ptr node,
                                          std::unordered_set<std::string> const& funcs,
                                          arg_map_t const* map) {
        EXPR(value);
        return node;
    }

    xcml::node_ptr visit_function_call(xcml::function_call_ptr node,
                                       std::unordered_set<std::string> const& funcs,
                                       arg_map_t const* map,
                                       xcml::compound_stmt_ptr const& c = nullptr) {
        if (auto fa = std::dynamic_pointer_cast<xcml::func_addr>(node->function)) {
            // fmt::print(stderr, "found static function call: `{}`\n", fa->name);

            if (funcs.find(fa->name) != funcs.end()) {
                // fmt::print(stderr, "    to be inlined\n");

                auto fd = get_function_definition(prg_, fa->name);

                if (c) {
                    do_inline(node, fd, funcs, map, c);
                    return nullptr;
                }

                return do_inline(node, fd, funcs, map);
            }
        } else {
            // fmt::print(stderr, "found dynamic function call\n");
        }

        for (size_t i = 0, n = node->arguments.size(); i < n; i++) {
            auto arg = node->arguments.at(i);
            arg = as_expr(visit(arg, funcs, map));
            node->arguments.at(i) = arg;
        }

        return node;
    }

    xcml::node_ptr visit_int_constant(xcml::int_constant_ptr node,
                                      std::unordered_set<std::string> const&,
                                      arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_string_constant(xcml::string_constant_ptr node,
                                         std::unordered_set<std::string> const&,
                                         arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_float_constant(xcml::float_constant_ptr node,
                                        std::unordered_set<std::string> const&,
                                        arg_map_t const*) {
        return node;
    }

    xcml::node_ptr visit_cast_expr(xcml::cast_expr_ptr node,
                                   std::unordered_set<std::string> const& funcs,
                                   arg_map_t const* map) {
        EXPR(value);
        return node;
    }

    arg_map_t make_argmap(xcml::function_call_ptr fc, xcml::function_definition_ptr fd,
                          std::unordered_set<std::string> const& funcs, arg_map_t const* map) {
        arg_map_t arg_map;

        for (size_t i = 0, n = fd->params.size(); i < n; i++) {
            auto p = fd->params.at(i);
            auto arg = as_expr(visit(fc->arguments.at(i), funcs, map));

            // fmt::print(stderr, "    remap {}\n", param->name);
            // dump_node(arg);

            if (auto param = xcml::param_node::dyncast(p)) {
                if (is_simple_ref(arg)) {
                    arg_map.insert_or_assign(param->name, arg);
                } else {
                    // TODO:
                    arg_map.insert_or_assign(param->name, arg);
                }
            }
        }

        return arg_map;
    }

    void do_inline(xcml::function_call_ptr fc, xcml::function_definition_ptr fd,
                   std::unordered_set<std::string> const& funcs, arg_map_t const* map0,
                   xcml::compound_stmt_ptr const& c) {
        auto const arg_map = make_argmap(fc, fd, funcs, map0);

        *c = *xcml::compound_stmt::dyncast(
            visit(xcml::clone_compound(fd->body), funcs, &arg_map));
    }

    xcml::node_ptr do_inline(xcml::function_call_ptr fc, xcml::function_definition_ptr fd,
                             std::unordered_set<std::string> const& funcs,
                             arg_map_t const* map0) {
        auto const arg_map = make_argmap(fc, fd, funcs, map0);

        auto body = skip(fd->body);

        if (body->node_name() != "return_stmt") {
            if (ignore_failed_) {
                return fc;
            }

            fmt::print(stderr, "Error: cannot inline function: {}\n", fd->name);
            dump_node(body);
            exit(1);
        }

        auto value =
            xcml::copy_expr(prg_, std::dynamic_pointer_cast<xcml::return_stmt>(body)->value);

        return visit(value, funcs, &arg_map);
    }

private:
    bool is_simple_ref(xcml::node_ptr nd) {
        return std::dynamic_pointer_cast<xcml::var_ref>(nd) ||
               std::dynamic_pointer_cast<xcml::func_addr>(nd) ||
               std::dynamic_pointer_cast<xcml::var_addr>(nd) ||
               std::dynamic_pointer_cast<xcml::array_addr>(nd) ||
               std::dynamic_pointer_cast<xcml::int_constant>(nd) ||
               std::dynamic_pointer_cast<xcml::float_constant>(nd) ||
               std::dynamic_pointer_cast<xcml::string_constant>(nd);
    }

    xcml::stmt_ptr skip(xcml::stmt_ptr const& stmt) {
        if (auto c = xcml::compound_stmt::dyncast(stmt)) {
            if (c->symbols.empty() && c->declarations.empty() && c->body.size() == 1) {
                return skip(c->body.front());
            }
        }
        return stmt;
    }

    xcml::xcml_program_node_ptr prg_;
    bool ignore_failed_;
};

struct collect_functions : xcml::recursive_visitor<collect_functions> {
    xcml::node_ptr visit_function_decl(xcml::function_decl_ptr const& node, scope_ref) {
        if (node->force_inline) {
            funcs_.insert(node->name);
        }
        return node;
    }

    std::unordered_set<std::string> const& get() const {
        return funcs_;
    }

private:
    std::unordered_set<std::string> funcs_;
};

[[maybe_unused]] xcml::xcml_program_node_ptr inline_functions(
    xcml::xcml_program_node_ptr const& prg, bool ignore_failed) {
    if (auto const* no_inline = getenv("CHARM_SYCL_NO_INLINE");
        no_inline && strlen(no_inline) > 0 && strcmp(no_inline, "0") != 0) {
        return prg;
    }

    collect_functions collector;
    collector.apply(prg);
    auto const& funcs = collector.get();

    for (auto decl : prg->global_declarations) {
        if (auto fd = std::dynamic_pointer_cast<xcml::function_definition>(decl)) {
            inliner{ignore_failed}(prg, fd, funcs);
        }
    }

    return prg;
}

}  // namespace

xcml::xcml_program_node_ptr inline_functions(xcml::xcml_program_node_ptr const& prg) {
    // return inline_functions(prg, true);
    return prg;
}

xcml::xcml_program_node_ptr inline_functions_mandatory(xcml::xcml_program_node_ptr const& prg) {
    // return inline_functions(prg, false);
    return prg;
}
