#include <fstream>
#include <iostream>
#include <vector>
#include <fmt/format.h>

static std::vector<char> buffer;

template <class... Args>
void pr(char const* fmt, Args&&... args) {
    fmt::format_to(std::back_inserter(buffer), fmt::runtime(fmt), std::forward<Args>(args)...);
}

static void sep() {
    pr("\n\n");
}

void gen_decl(int dim) {
    pr("namespace detail {{");
    sep();
    pr("inline item<{}> make_item(range<{}> const& r, id<{}> const& i);", dim, dim, dim);
    sep();
    pr("}}");
    sep();

    pr("template <> struct item<{}, false> {{", dim);

    pr("item() = delete;");
    sep();

    pr("inline id<{}> CHARM_SYCL_INLINE get_id() const;", dim);
    sep();

    pr("inline size_t CHARM_SYCL_INLINE get_id(int dimension) const;");
    sep();

    pr("inline size_t CHARM_SYCL_INLINE operator[](int dimension) const;");
    sep();

    pr("inline range<{}> CHARM_SYCL_INLINE get_range() const;", dim);
    sep();

    pr("inline size_t CHARM_SYCL_INLINE get_range(int dimension) const;");
    sep();

    if (dim == 1) {
        pr("inline CHARM_SYCL_INLINE operator size_t() const;");
        sep();
    }

    pr("inline size_t CHARM_SYCL_INLINE get_linear_id() const;");
    sep();

    pr("private:");

    pr("friend item<{}> detail::make_item(range<{}> const& r, id<{}> const& i);", dim, dim,
       dim);
    sep();

    pr("inline item(range<{}> const& r, id<{}> const& i);", dim, dim);
    sep();

    pr("id<{}> id_; range<{}> range_;", dim, dim);

    pr("}};");
    sep();
}

void gen_impl(int dim) {
    pr("namespace detail {{");
    sep();
    pr("item<{}> make_item(range<{}> const& r, id<{}> const& i) {{ return "
       "item<{}>(r,i); }}",
       dim, dim, dim, dim);
    sep();
    pr("}}");
    sep();

    pr("id<{}> item<{}>::get_id() const{{ return id_; }}", dim, dim);
    sep();

    pr("size_t item<{}>::get_id(int dimension) const {{ return id_[dimension]; }}", dim);
    sep();

    pr("size_t item<{}>::operator[](int dimension) const {{ return id_[dimension]; }}", dim);
    sep();

    pr("range<{}> item<{}>::get_range() const {{ return range_; }}", dim, dim);
    sep();

    pr("size_t item<{}>::get_range(int dimension) const {{ return range_[dimension]; }}", dim);
    sep();

    if (dim == 1) {
        pr("item<{}>::operator size_t() const {{ return id_[0]; }}", dim);
        sep();
    }

    pr("size_t item<{}>::get_linear_id() const {{", dim);
    switch (dim) {
        case 1:
            pr("return id_[0];");
            break;
        case 2:
            pr("return id_[1] + id_[0] * range_[1];");
            break;
        default:
            pr("return id_[2] + id_[1] * range_[2] + id_[0] * range_[1] * range_[2];");
    }
    pr("}}");
    sep();

    pr("item<{}>::item(range<{}> const& r, id<{}> const& i) : id_(i), range_(r) {{}}", dim, dim,
       dim);
    sep();
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

    if (mode == "item-decl") {
        for (int dim = 1; dim <= 3; ++dim) {
            gen_decl(dim);
        }
    } else if (mode == "item-impl") {
        for (int dim = 1; dim <= 3; ++dim) {
            gen_impl(dim);
        }
    }

    pr("CHARM_SYCL_END_NAMESPACE");
    sep();

    if (argc == 3) {
        std::ofstream ofs(argv[2]);
        ofs.write(buffer.data(), buffer.size());
    } else {
        std::cout.write(buffer.data(), buffer.size());
    }

    return 0;
}
