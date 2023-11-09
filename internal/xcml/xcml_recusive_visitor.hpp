#pragma once

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <utils/naming.hpp>
#include <xcml_type.hpp>
#include <xcml_utils.hpp>
#include <xcml_visitor.hpp>

namespace xcml {

struct symbol_scope {
    template <class Symbols>
    explicit symbol_scope(std::unordered_map<std::string, type_ptr> const& type,
                          Symbols const& syms)
        : type_map_(type) {
        init(syms);
    }

    template <class Symbols>
    explicit symbol_scope(symbol_scope const& parent, Symbols const& syms)
        : type_map_(parent.type_map_) {
        tab_ = parent.tab_;
        typetab_ = parent.typetab_;
        init(syms);
    }

    struct lookup_result {
        symbol_id_ptr const sym;
        std::shared_ptr<type_node> const type;

        function_type_ptr function_type() const {
            if (auto ft = function_type::dyncast(type)) {
                return ft;
            }
            fmt::print(stderr, "not a function type: {} referenced as symbol {}", type->type,
                       sym->name);
            std::exit(1);
        }
    };

    bool exists(std::string const& name) const {
        return tab_.find(name) != tab_.end();
    }

    lookup_result lookup(std::string const& name) const {
        auto const sym = tab_.find(name);
        if (sym == tab_.end()) {
            fmt::print(stderr, "symbol not found: {}", name);
            std::exit(1);
        }

        auto const type = type_map_.find(sym->second->type);
        if (type == type_map_.end()) {
            fmt::print(stderr, "type not found: {} referenced as symbol {}", sym->second->type,
                       name);
            std::exit(1);
        }

        return {sym->second, type->second};
    }

    lookup_result lookup_by_type(std::string const& name) const {
        auto const sym = typetab_.find(name);
        if (sym == typetab_.end()) {
            fmt::print(stderr, "type not found: {}", name);
            std::exit(1);
        }

        auto const type = type_map_.find(sym->second->type);
        if (type == type_map_.end()) {
            fmt::print(stderr, "type not found: {} referenced as type {}", sym->second->type,
                       name);
            std::exit(1);
        }

        return {sym->second, type->second};
    }

private:
    template <class Symbols>
    void init(Symbols const& syms) {
        for (auto const& sym : syms) {
            if (sym->sclass == storage_class::tagname) {
                typetab_.insert_or_assign(sym->type, sym);
            } else {
                tab_.insert_or_assign(sym->name, sym);
            }
        }
    }

    std::unordered_map<std::string, symbol_id_ptr> tab_;
    std::unordered_map<std::string, symbol_id_ptr> typetab_;
    std::unordered_map<std::string, type_ptr> const& type_map_;
};

struct recursive_visitor_base : protected ::utils::naming_utils {
    using scope_ref = std::optional<std::reference_wrapper<symbol_scope>>;

protected:
    void init(xcml_program_node_ptr const& node);

    void fini(xcml_program_node_ptr const& node);

    xcml::xcml_program_node_ptr const& root();

    symbol_scope::lookup_result lookup(scope_ref scope, std::string const& name);

    basic_type_ptr create_basic_type(std::string const& name, bool is_const, bool is_builtin);

    pointer_type_ptr create_pointer_type(type_ptr const& ref);

    pointer_type_ptr create_pointer_type(std::string const& ref);

    function_type_ptr create_function_type();

    type_ptr find_pointer_type(type_ptr const& type) const;

    type_ptr get_pointer_type(type_ptr const& type);

    type_ptr const& get_basic_type(std::string const& name) const;

    size_t nextid();

    type_ptr rts_accessor_type();

    type_ptr rts_accessor_ptr_type();

    void add_type(std::shared_ptr<type_node> const& node);

    std::string compute_type(expr_ptr const& expr, symbol_scope const& scope);

    type_ptr const& get_type(std::string const&) const;

    xcml::xcml_program_node_ptr root_;
    std::unordered_map<std::string, type_ptr> type_map_;
};

template <class Derived>
struct recursive_visitor : visitor<Derived, node_ptr>, recursive_visitor_base {
    xcml_program_node_ptr operator()(xcml_program_node_ptr const& node) {
        return apply(node);
    }

    xcml_program_node_ptr apply(xcml_program_node_ptr const& node) {
        auto prg = node;
        init(prg);
        prg = xcml_program_node::dyncast(this->visit(prg, std::nullopt));
        fini(prg);
        return prg;
    }

    void visit_xcml_program_node_1(xcml_program_node_ptr const& node) {
        for (auto& type : node->type_table) {
            type = type_node::dyncast(this->visit(type, std::nullopt));
            type_map_.insert_or_assign(type->type, type);
        }
    }

    xcml_program_node_ptr visit_xcml_program_node_2(xcml_program_node_ptr const& node) {
        symbol_scope scope(type_map_, node->global_symbols);

        for (auto& decl : node->global_declarations) {
            decl = decl_node::dyncast(this->visit(decl, scope));
        }
        return node;
    }

    node_ptr visit_xcml_program_node(xcml_program_node_ptr const& node, scope_ref) {
        visit_xcml_program_node_1(node);
        return visit_xcml_program_node_2(node);
    }

    node_ptr visit_function_type(function_type_ptr const& node, scope_ref) {
        add_type(node);
        return node;
    }

    node_ptr visit_pointer_type(pointer_type_ptr const& node, scope_ref) {
        add_type(node);
        return node;
    }

    node_ptr visit_struct_type(struct_type_ptr const& node, scope_ref) {
        add_type(node);
        return node;
    }

    node_ptr visit_basic_type(basic_type_ptr const& node, scope_ref) {
        add_type(node);
        return node;
    }

    node_ptr visit_array_type(array_type_ptr const& node, scope_ref) {
        add_type(node);
        return node;
    }

    node_ptr visit_function_decl(function_decl_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_runtime_func_decl(runtime_func_decl_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_cpp_include(cpp_include_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_var_decl(var_decl_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_function_definition(function_definition_ptr const& node, scope_ref scope) {
        symbol_scope new_scope(scope.value(), node->symbols);
        stmt_(node->body, new_scope);
        return node;
    }

    xcml::node_ptr visit_kernel_wrapper_decl(xcml::kernel_wrapper_decl_ptr const& node,
                                             scope_ref scope) {
        stmt_(node->body, scope);
        return node;
    }

    node_ptr visit_compound_stmt(compound_stmt_ptr const& node, scope_ref scope) {
        symbol_scope new_scope(scope.value(), node->symbols);

        for (auto& decl : node->declarations) {
            decl_(decl, scope);
        }

        for (auto& stmt : node->body) {
            stmt_(stmt, new_scope);
        }
        return node;
    }

    node_ptr visit_expr_stmt(expr_stmt_ptr const& node, scope_ref scope) {
        expr_(node->expr, scope);
        return node;
    }

    node_ptr visit_for_stmt(for_stmt_ptr const& node, scope_ref scope) {
        if (node->init) {
            expr_(node->init, scope);
        }
        if (node->condition) {
            expr_(node->condition, scope);
        }
        if (node->iter) {
            expr_(node->iter, scope);
        }
        if (node->body) {
            stmt_(node->body, scope);
        }
        return node;
    }

    node_ptr visit_if_stmt(if_stmt_ptr const& node, scope_ref scope) {
        expr_(node->condition, scope);
        stmt_(node->then, scope);
        stmt_(node->else_, scope);
        return node;
    }

    node_ptr visit_parallel_invoke(parallel_invoke_ptr const& node, scope_ref scope) {
        for (auto& dim : node->dimensions) {
            expr_(dim->offset, scope);
            expr_(dim->size, scope);
        }
        expr_(node->function, scope);
        for (auto& arg : node->arguments) {
            expr_(arg, scope);
        }
        return node;
    }

    node_ptr visit_ndr_invoke(ndr_invoke_ptr const& node, scope_ref scope) {
        for (auto& dim : node->group) {
            expr_(dim->offset, scope);
            expr_(dim->size, scope);
        }
        for (auto& dim : node->local) {
            expr_(dim->offset, scope);
            expr_(dim->size, scope);
        }
        expr_(node->function, scope);
        for (auto& arg : node->arguments) {
            expr_(arg, scope);
        }
        return node;
    }

    node_ptr visit_return_stmt(return_stmt_ptr const& node, scope_ref scope) {
        if (node->value) {
            expr_(node->value, scope);
        }
        return node;
    }

    node_ptr visit_continue_stmt(continue_stmt_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_break_stmt(break_stmt_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_binary_op(std::shared_ptr<binary_op> const& node, scope_ref scope) {
        expr_(node->lhs, scope);
        expr_(node->rhs, scope);
        return node;
    }

    node_ptr visit_unary_op(std::shared_ptr<unary_op> const& node, scope_ref scope) {
        expr_(node->expr, scope);
        return node;
    }

    node_ptr visit_pointer_ref(pointer_ref_ptr const& node, scope_ref scope) {
        expr_(node->expr, scope);
        return node;
    }

    node_ptr visit_member_ref(member_ref_ptr const& node, scope_ref scope) {
        expr_(node->value, scope);
        return node;
    }

    node_ptr visit_member_array_ref(member_array_ref_ptr const& node, scope_ref scope) {
        expr_(node->value, scope);
        return node;
    }

    node_ptr visit_var_ref(var_ref_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_var_addr(var_addr_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_array_ref(array_ref_ptr const& node, scope_ref scope) {
        expr_(node->array, scope);
        for (auto& idx : node->index) {
            expr_(idx, scope);
        }
        return node;
    }

    node_ptr visit_member_addr(member_addr_ptr const& node, scope_ref scope) {
        expr_(node->value, scope);
        return node;
    }

    node_ptr visit_cast_expr(cast_expr_ptr const& node, scope_ref scope) {
        expr_(node->value, scope);
        return node;
    }

    node_ptr visit_int_constant(int_constant_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_long_long_constant(long_long_constant_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_float_constant(float_constant_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_string_constant(string_constant_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_func_addr(func_addr_ptr const& node, scope_ref) {
        return node;
    }

    node_ptr visit_function_call(function_call_ptr const& node, scope_ref scope) {
        expr_(node->function, scope);
        for (auto& arg : node->arguments) {
            expr_(arg, scope);
        }
        return node;
    }

    node_ptr visit_cond_expr(cond_expr_ptr const& node, scope_ref scope) {
        expr_(node->cond, scope);
        expr_(node->true_, scope);
        expr_(node->false_, scope);
        return node;
    }

    node_ptr visit_switch_stmt(switch_stmt_ptr const& node, scope_ref scope) {
        expr_(node->value, scope);
        stmt_(node->body, scope);
        return node;
    }

    node_ptr visit_case_label_stmt(case_label_stmt_ptr const& node, scope_ref scope) {
        expr_(node->value, scope);
        return node;
    }

    node_ptr visit_default_stmt(default_stmt_ptr const& node, scope_ref) {
        return node;
    }

protected:
    void expr_(expr_ptr& node, scope_ref scope) {
        node = expr_node::dyncast(this->visit(node, scope));
    }

    void decl_(decl_ptr& node, scope_ref scope) {
        node = decl_node::dyncast(this->visit(node, scope));
    }

    template <class Stmt>
    void stmt_(std::shared_ptr<Stmt>& node, scope_ref scope) {
        static_assert(std::is_base_of_v<stmt_node, Stmt>);
        node = Stmt::dyncast(this->visit(node, scope));
    }
};

}  // namespace xcml
