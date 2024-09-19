#include <cstdint>
#include <functional>
#include <sstream>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <utils/errors.hpp>
#include <utils/io.hpp>

namespace io = utils::io;

namespace {

template <class F>
int try_handle_all(char const* argv0, F&& f) {
    return boost::leaf::try_handle_all(
        [&]() -> boost::leaf::result<int> {
            return utils::io::try_handle_some(argv0, [&]() -> boost::leaf::result<int> {
                return utils::errors::try_handle_some(argv0, f);
            });
        },
        [&](boost::leaf::verbose_diagnostic_info const& diag) -> int {
            utils::errors::report_and_exit(argv0, diag);
        });
}

boost::leaf::result<int> main_impl(int argc, char** argv) {
    if (argc != 4) {
        fmt::print(stderr, "Invalid Argument: {} input-file prefix output-file\n", argv[0]);
        return 1;
    }

    auto prefix = std::string(argv[2]);
    auto output = std::string(argv[3]);

    auto input = BOOST_LEAF_CHECK(io::file::readonly(argv[1]));
    auto st = BOOST_LEAF_CHECK(input.stat());
    auto const size = static_cast<uint64_t>(st.st_size);
    std::stringstream buffer;

#ifdef BIN2ASM_USE_MACHO
    fmt::print(buffer, ".section __data,__DATA\n");
#else
    fmt::print(buffer, ".section .rodata\n");
#endif

    fmt::print(buffer, R"(
                .align 64
                .global {}_start
                {}_start:
                    .incbin "{}"
                    .byte 0
                
                .align 8
                .global {}_size
                {}_size:
                    .byte {}, {}, {}, {}, {}, {}, {}, {}
                )",
               prefix, prefix, input.filename(), prefix, prefix, (size >> 0) & 0xff,
               (size >> 8) & 0xff, (size >> 16) & 0xff, (size >> 24) & 0xff,
               (size >> 32) & 0xff, (size >> 40) & 0xff, (size >> 48) & 0xff,
               (size >> 56) & 0xff);

    std::string data(std::move(buffer).str());

    if (output == "-") {
        fmt::print("{}", data);
    } else {
        auto file = BOOST_LEAF_CHECK(io::file::create(output));
        BOOST_LEAF_CHECK(file.write_str(0, data));
    }

    return 0;
}

}  // namespace

#ifdef IMPLEMENT_MAIN
int main(int argc, char** argv)
#else
int bin2asm_main(int argc, char** argv)
#endif
{
    return try_handle_all(argv[0], std::bind(main_impl, argc, argv));
}
