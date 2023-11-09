#include <fstream>
#include <iostream>
#include <vector>
#include <fmt/format.h>

static std::vector<char> buffer;

template <class... Args>
void pr(char const* fmt, Args&&... args) {
    fmt::format_to(std::back_inserter(buffer), fmt::runtime(fmt), std::forward<Args>(args)...);
}

static void comma(bool cond, char const* op = nullptr) {
    if (cond) {
        if (op) {
            pr("{}", op);
        } else {
            pr(",");
        }
    }
}

static void sep() {
    pr("\n\n");
}

void gen_decl(std::string_view klass, int dim) {
    pr("template <> struct {}<{}> {{", klass, dim);

    if (klass == "id") {
        pr("inline {}();", klass);
        sep();
    }

    pr("inline {}(", klass);
    for (int i = 0; i < dim; i++) {
        comma(i > 0);
        pr("size_t dim{}", i);
    }
    pr(");");
    sep();

    if (klass == "id") {
        pr("inline {}(range<{}> const&);", klass, dim);
        sep();

        pr("inline {}(item<{}> const&);", klass, dim);
        sep();
    }

    if (klass == "id" && dim == 1) {
        pr("inline operator size_t() const;");
        sep();
    }

    pr("inline CHARM_SYCL_INLINE size_t size() const;");
    sep();
    pr("inline CHARM_SYCL_INLINE size_t get(size_t dimension) const;");
    sep();
    pr("inline CHARM_SYCL_INLINE size_t& operator [](size_t dimension);");
    sep();
    pr("inline CHARM_SYCL_INLINE size_t const& operator [](size_t dimension) const;");
    sep();

    for (std::string_view op : {"==", "!="}) {
        pr("inline CHARM_SYCL_INLINE friend bool operator {}({}<{}> const& lhs, {}<{}> const& "
           "rhs);",
           op, klass, dim, klass, dim);
        sep();
    }

    for (std::string_view op : {"+", "-", "*", "/", "%", "<<", ">>", "&", "|", "^", "&&", "||",
                                "<", ">", "<=", ">="}) {
        pr("inline CHARM_SYCL_INLINE friend {}<{}> operator {}({}<{}> const& lhs, {}<{}> "
           "const& rhs);",
           klass, dim, op, klass, dim, klass, dim);
        sep();

        pr("inline CHARM_SYCL_INLINE friend {}<{}> operator {}({}<{}> const& lhs, size_t rhs);",
           klass, dim, op, klass, dim);
        sep();

        pr("inline CHARM_SYCL_INLINE friend {}<{}> operator {}(size_t lhs, {}<{}> const& rhs);",
           klass, dim, op, klass, dim);
        sep();
    }

    for (std::string_view op : {"+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "|=", "^="}) {
        pr("inline CHARM_SYCL_INLINE friend {}<{}>& operator {}({}<{}>& lhs, {}<{}> const& "
           "rhs);",
           klass, dim, op, klass, dim, klass, dim);
        sep();

        pr("inline CHARM_SYCL_INLINE friend {}<{}>& operator {}({}<{}>& lhs, size_t rhs);",
           klass, dim, op, klass, dim);
        sep();
    }

    pr("private:");
    pr("size_t {}_[{}];", klass, dim);
    pr("}};");
    sep();

    if (klass == "range" || klass == "id") {
        switch (dim) {
            case 1:
                pr("{}(size_t)->{}<1>;", klass, klass);
                break;
            case 2:
                pr("{}(size_t, size_t)->{}<2>;", klass, klass);
                break;
            default:
                pr("{}(size_t, size_t, size_t)->{}<3>;", klass, klass);
                break;
        }
        sep();
    }
}

void gen_impl(std::string_view klass, int dim) {
    auto const type = fmt::format("{}<{}>", klass, dim);

    if (klass == "id") {
        pr("{}::{}() {{", type, klass);
        for (int i = 0; i < dim; i++) {
            pr("{}_[{}] = 0;", klass, i);
        }
        pr("}}");
        sep();
    }

    pr("{}::{}(", type, klass);
    for (int i = 0; i < dim; i++) {
        comma(i > 0);
        pr("size_t dim{}", i);
    }
    pr(") {{");
    for (int i = 0; i < dim; i++) {
        pr("{}_[{}] = dim{};", klass, i, i);
    }
    pr("}}");
    sep();

    if (klass == "id") {
        pr("{}::{}(range<{}> const& r) {{", type, klass, dim);
        for (int i = 0; i < dim; i++) {
            pr("{}_[{}] = r[{}];", klass, i, i);
        }
        pr("}}");
        sep();

        pr("{}::{}(item<{}> const& i) {{", type, klass, dim);
        for (int i = 0; i < dim; i++) {
            pr("{}_[{}] = i[{}];", klass, i, i);
        }
        pr("}}");
        sep();
    }

    if (klass == "id" && dim == 1) {
        pr("{}::operator size_t() const {{ return {}_[0]; }}", type, klass);
        sep();
    }

    pr("size_t {}::size() const {{ return ", type);
    for (int i = 0; i < dim; i++) {
        comma(i > 0, "*");
        pr("{}_[{}]", klass, i);
    }
    pr("; }}", type);
    sep();

    pr("size_t {}::get(size_t dimension) const {{ return {}_[dimension]; }}", type, klass);
    sep();

    pr("size_t& {}::operator [](size_t dimension) {{ return {}_[dimension]; }}", type, klass);
    sep();

    pr("size_t const& {}::operator [](size_t dimension) const {{ return {}_[dimension]; }}",
       type, klass);
    sep();

    for (std::string_view op : {"==", "!="}) {
        pr("bool operator {}({}<{}> const& lhs, {}<{}> const& rhs)", op, klass, dim, klass,
           dim);
        pr("{{ return ");
        for (int i = 0; i < dim; i++) {
            comma(i > 0, "&&");
            pr("lhs.{}_[{}] {} rhs.{}_[{}]", klass, i, op, klass, i);
        }
        pr("; }}");
        sep();
    }

    for (std::string_view op : {"+", "-", "*", "/", "%", "<<", ">>", "&", "|", "^", "&&", "||",
                                "<", ">", "<=", ">="}) {
        pr("{}<{}> operator {}({}<{}> const& lhs, {}<{}> const& rhs)", klass, dim, op, klass,
           dim, klass, dim);
        pr("{{ return {}<{}>(", klass, dim);
        for (int i = 0; i < dim; i++) {
            comma(i > 0);
            pr("lhs.{}_[{}] {} rhs.{}_[{}]", klass, i, op, klass, i);
        }
        pr("); }}");
        sep();

        pr("inline {}<{}> operator {}({}<{}> const& lhs, size_t rhs)", klass, dim, op, klass,
           dim);
        pr("{{ return {}<{}>(", klass, dim);
        for (int i = 0; i < dim; i++) {
            comma(i > 0);
            pr("lhs.{}_[{}] {} rhs", klass, i, op);
        }
        pr("); }}");
        sep();

        pr("inline {}<{}> operator {}(size_t lhs, {}<{}> const& rhs)", klass, dim, op, klass,
           dim);
        pr("{{ return {}<{}>(", klass, dim);
        for (int i = 0; i < dim; i++) {
            comma(i > 0);
            pr("lhs {} rhs.{}_[{}]", op, klass, i);
        }
        pr("); }}");
        sep();
    }

    for (std::string_view op : {"+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "|=", "^="}) {
        pr("{}<{}>& operator {}({}<{}>& lhs, {}<{}> const& rhs) {{", klass, dim, op, klass, dim,
           klass, dim);
        for (int i = 0; i < dim; i++) {
            pr("lhs.{}_[{}] {} rhs.{}_[{}];", klass, i, op, klass, i);
        }
        pr("return lhs; }}");
        sep();

        pr("inline {}<{}>& operator {}({}<{}>& lhs, size_t rhs) {{", klass, dim, op, klass,
           dim);
        for (int i = 0; i < dim; i++) {
            pr("lhs.{}_[{}] {} rhs;", klass, i, op);
        }
        pr("return lhs; }}");
        sep();
    }
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        return 1;
    }

    pr("#pragma once");
    sep();
    pr("#include <charm/sycl.hpp>\n");
    sep();
    pr("CHARM_SYCL_BEGIN_NAMESPACE");
    sep();

    auto const mode = std::string_view(argv[1]);

    if (mode == "id-decl") {
        for (int dim = 1; dim <= 3; ++dim) {
            gen_decl("id", dim);
        }
    } else if (mode == "id-impl") {
        for (int dim = 1; dim <= 3; ++dim) {
            gen_impl("id", dim);
        }
    } else if (mode == "range-decl") {
        for (int dim = 1; dim <= 3; ++dim) {
            gen_decl("range", dim);
        }
    } else if (mode == "range-impl") {
        for (int dim = 1; dim <= 3; ++dim) {
            gen_impl("range", dim);
        }
    }

    pr("CHARM_SYCL_END_NAMESPACE");

    if (argc == 3) {
        std::ofstream ofs(argv[2]);
        ofs.write(buffer.data(), buffer.size());
    } else {
        std::cout.write(buffer.data(), buffer.size());
    }

    return 0;
}
