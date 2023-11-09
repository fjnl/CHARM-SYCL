#include <fstream>
#include <iostream>
#include <vector>
#include <fmt/compile.h>
#include <fmt/format.h>

static std::vector<char> buffer;

#define PRINT(str) fmt::format_to(std::back_inserter(buffer), FMT_COMPILE("{}"), (str))

#define FORMAT(fmtstr, ...) \
    fmt::format_to(std::back_inserter(buffer), FMT_COMPILE(fmtstr), __VA_ARGS__)

#define SEP() PRINT("\n\n")

struct ftype {
    explicit ftype(std::string_view basename, int vec)
        : basename_(basename),
          name_(vec == 1 ? basename : fmt::format(FMT_COMPILE("{}{}"), basename, vec)),
          vec_(vec) {}

    operator std::string_view() const {
        return name();
    }

    std::string const& name() const {
        return name_;
    }

    int veclen() const {
        return vec_;
    }

    bool is_vec() const {
        return veclen() > 1;
    }

    ftype scalar() const {
        return ftype(basename_, 1);
    }

private:
    std::string basename_;
    std::string name_;
    int vec_;
};

void fvectors(std::vector<ftype>& vec, std::string_view name) {
    vec.emplace_back(name, 1);
    for (auto n : {2, 3, 4, 8, 16}) {
        vec.emplace_back(name, n);
    }
}

struct itype {
    explicit itype(std::string_view basename, bool sign, int vec)
        : basename_(basename),
          name_(sign ? basename : "unsigned " + std::string(basename)),
          sign_(sign),
          vec_(vec) {}

    operator std::string_view() const {
        return name();
    }

    std::string const& name() const {
        return name_;
    }

    bool is_signed() const {
        return sign_;
    }

    bool is_unsigned() const {
        return !is_signed();
    }

    itype to_signed() const {
        if (is_signed()) {
            return *this;
        }
        return itype(basename_, true, vec_);
    }

    itype to_unsigned() const {
        if (is_unsigned()) {
            return *this;
        }
        return itype(basename_, false, vec_);
    }

    int veclen() const {
        return vec_;
    }

    bool is_vec() const {
        return veclen() > 1;
    }

private:
    std::string basename_, name_;
    bool sign_;
    int vec_;
};

struct context {
    ftype const* genfloat = nullptr;
    ftype const* gengeofloat = nullptr;
    itype const* geninteger = nullptr;
    itype const* sgeninteger = nullptr;
};

#define defun(return_type, name, args_fmt, ...)                                          \
    ({                                                                                   \
        PRINT("#ifdef __SYCL_DEVICE_ONLY__\n");                                          \
        FORMAT("CHARM_SYCL_EXTERN_RUNTIME CHARM_SYCL_INLINE {} {}(", return_type, name); \
        fmt::format_to(std::back_inserter(buffer), FMT_COMPILE(args_fmt), __VA_ARGS__);  \
        PRINT(");\n");                                                                   \
        PRINT("#else\n");                                                                \
        FORMAT("extern {} {}(", return_type, name);                                      \
        fmt::format_to(std::back_inserter(buffer), FMT_COMPILE(args_fmt), __VA_ARGS__);  \
        PRINT(");\n");                                                                   \
        PRINT("#endif\n\n");                                                             \
    })

void generate(context const& c) {
    if (c.genfloat && !c.geninteger) {
        defun(*c.genfloat, "cbrt", "{} x", *c.genfloat);
        defun(*c.genfloat, "cos", "{} x", *c.genfloat);
        defun(*c.genfloat, "exp", "{} x", *c.genfloat);
        defun(*c.genfloat, "fabs", "{} x", *c.genfloat);
        defun(*c.genfloat, "sin", "{} x", *c.genfloat);
        defun(*c.genfloat, "sqrt", "{} x", *c.genfloat);
        defun(*c.genfloat, "tan", "{} x", *c.genfloat);

        defun(*c.genfloat, "fdim", "{} x, {} y", *c.genfloat, *c.genfloat);
        defun(*c.genfloat, "fmax", "{} x, {} y", *c.genfloat, *c.genfloat);
        defun(*c.genfloat, "fmin", "{} x, {} y", *c.genfloat, *c.genfloat);
        defun(*c.genfloat, "hypot", "{} x, {} y", *c.genfloat, *c.genfloat);
        defun(*c.genfloat, "max", "{} x, {} y", *c.genfloat, *c.genfloat);
        defun(*c.genfloat, "min", "{} x, {} y", *c.genfloat, *c.genfloat);

        defun(*c.genfloat, "clamp", "{} x, {} minval, {} maxval", *c.genfloat, *c.genfloat,
              *c.genfloat);
    }

    if (c.gengeofloat) {
        defun(c.gengeofloat->scalar(), "length", "{} x", *c.gengeofloat, *c.gengeofloat);
    }
}

void gen_funcs() {
    std::vector<itype> genintegers;
    std::vector<ftype> genfloats;

    for (auto const* basename : {"char", "short", "int", "long", "long long"}) {
        genintegers.emplace_back(basename, false, 1);
        genintegers.emplace_back(basename, true, 1);
    }
    for (auto const* name : {"float", "double"}) {
        fvectors(genfloats, name);
    }

    for (auto const& geni : genintegers) {
        context c;
        auto const sgeni = geni.to_signed();

        c.geninteger = &geni;
        if (geni.is_unsigned()) {
            c.sgeninteger = &sgeni;
        }

        generate(c);
    }

    for (auto const& genf : genfloats) {
        context c;
        c.genfloat = &genf;

        if (genf.veclen() <= 4) {
            c.gengeofloat = &genf;
        }

        generate(c);
    }
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        return 1;
    }

    PRINT("#pragma once");
    SEP();
    PRINT("#include <charm/sycl.hpp>\n");
    SEP();
    PRINT("CHARM_SYCL_BEGIN_NAMESPACE");
    SEP();

    auto const mode = std::string_view(argv[1]);

    if (mode == "impl") {
        gen_funcs();
    } else {
        fmt::print(stderr, "Error: unknown: {}\n", mode);
        return 1;
    }

    PRINT("CHARM_SYCL_END_NAMESPACE");

    if (argc == 3) {
        std::ofstream ofs(argv[2]);
        ofs.write(buffer.data(), buffer.size());
    } else {
        std::cout.write(buffer.data(), buffer.size());
    }

    return 0;
}
