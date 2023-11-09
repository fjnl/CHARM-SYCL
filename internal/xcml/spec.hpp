#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

static constexpr uint32_t K_NODE = static_cast<uint32_t>(1) << 31;
static constexpr uint32_t K_STMT = K_NODE | static_cast<uint32_t>(1) << 30;
static constexpr uint32_t K_EXPR = K_NODE | K_STMT | static_cast<uint32_t>(1) << 29;
static constexpr uint32_t K_DECL = K_NODE | static_cast<uint32_t>(1) << 28;
static constexpr uint32_t K_TYPE = K_NODE | static_cast<uint32_t>(1) << 27;
static constexpr uint32_t K_UNARY = K_NODE | K_EXPR | static_cast<uint32_t>(1) << 26;
static constexpr uint32_t K_BINARY = K_NODE | K_EXPR | static_cast<uint32_t>(1) << 25;
static constexpr uint32_t K_PARAM = K_NODE | static_cast<uint32_t>(1) << 24;

struct spec {
    int type;
    std::string name;
    std::string tag;
    std::string op;
    std::vector<std::pair<std::string, std::string>> members;
    uint32_t id;

    std::string_view parent_type() const {
        if (is_node()) {
            return "node";
        }
        if (is_stmt()) {
            return "stmt_node";
        }
        if (is_expr()) {
            return "expr_node";
        }
        if (is_decl()) {
            return "decl_node";
        }
        if (is_type()) {
            return "type_node";
        }
        if (is_binary()) {
            return "binary_op";
        }
        if (is_unary()) {
            return "unary_op";
        }
        if (is_param()) {
            return "params_node";
        }
        abort();
    }

    uint32_t kind() const {
        if (is_stmt()) {
            return K_STMT | id;
        }
        if (is_expr()) {
            return K_EXPR | id;
        }
        if (is_decl()) {
            return K_DECL | id;
        }
        if (is_type()) {
            return K_TYPE | id;
        }
        if (is_binary()) {
            return K_BINARY | id;
        }
        if (is_unary()) {
            return K_UNARY | id;
        }
        if (is_param()) {
            return K_PARAM | id;
        }
        return K_NODE | id;
    }

    bool is_node() const {
        return type == (1 << 0);
    }

    bool is_stmt() const {
        return type == (1 << 1);
    }

    bool is_expr() const {
        return type == (1 << 2);
    }

    bool is_decl() const {
        return type == (1 << 3);
    }

    bool is_type() const {
        return type == (1 << 4);
    }

    bool is_binary() const {
        return type == (1 << 5);
    }

    bool is_unary() const {
        return type == (1 << 6);
    }

    bool is_param() const {
        return type == (1 << 7);
    }
};

std::vector<spec> load_specs();

struct xml_node {
    virtual ~xml_node() {}
    std::string var;
};

struct if_node : xml_node {
    std::vector<std::unique_ptr<xml_node>> body;
};

struct foreach_node : xml_node {};

struct ref_node : xml_node {};

struct node_node : xml_node {};

struct nodes_node : xml_node {};

struct attr {
    std::string var;
    bool test;
};

struct tag_node : xml_node {
    std::vector<attr> attrs;
    std::vector<std::unique_ptr<xml_node>> body;
};

struct xml_spec {
    std::unique_ptr<tag_node> top;
};

std::vector<xml_spec> load_xml_specs();
