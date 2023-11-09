#include <fmt/compile.h>
#include <fmt/format.h>

#define PRINT(str) fmt::print(FMT_COMPILE("{}"), (str))

#define FORMAT(fmt_str, ...) fmt::print(FMT_COMPILE(fmt_str), __VA_ARGS__)

#define SEP() PRINT("\n\n")

namespace {

void gen_vec(int num) {
    FORMAT("template <class DataT> struct vec<DataT, {}>", num);
    PRINT("{");

    PRINT("inline constexpr CHARM_SYCL_INLINE vec() : vec(DataT()) {}");
    SEP();

    PRINT("inline explicit constexpr CHARM_SYCL_INLINE vec(DataT const& arg) {");
    for (int i = 0; i < num; i++) {
        FORMAT("vec_[{}] = arg;", i);
    }
    PRINT("}");
    SEP();

    if (num == 1) {
        PRINT("inline operator CHARM_SYCL_INLINE DataT() const");
        PRINT("{ return vec_[0]; }");
    }

    PRINT("static inline constexpr size_t CHARM_SYCL_INLINE byte_size() noexcept");
    FORMAT("{{ return sizeof(DataT) * {}; }}", num);
    SEP();

    PRINT("static inline constexpr size_t CHARM_SYCL_INLINE size() noexcept");
    FORMAT("{{ return {}; }}", num);
    SEP();

    PRINT("inline size_t CHARM_SYCL_INLINE get_size() const");
    PRINT("{ return byte_size(); }");
    SEP();

    PRINT("inline size_t CHARM_SYCL_INLINE get_count() const");
    PRINT("{ return size(); }");
    SEP();

    PRINT("private:");
    FORMAT("DataT vec_[{}];", num);
    PRINT("};");
}

}  // namespace

int main() {
    // The vec<T, N> type must be transformable into devices by the compiler.
    // It is required for the compiler to support all AST nodes inside the type.

    PRINT("#pragma once\n");
    SEP();
    PRINT("#include <charm/sycl.hpp>\n");
    SEP();
    PRINT("CHARM_SYCL_BEGIN_NAMESPACE\n");
    SEP();
    PRINT("template <class DataT, int NumElements> struct vec;");
    SEP();
    gen_vec(1);
    SEP();
    gen_vec(2);
    SEP();
    gen_vec(3);
    SEP();
    gen_vec(4);
    SEP();
    gen_vec(8);
    SEP();
    gen_vec(16);
    SEP();
    PRINT("CHARM_SYCL_END_NAMESPACE\n");

    return 0;
}
