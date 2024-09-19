#include <utils/hash.hpp>
#include <utils/io.hpp>
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

result<io::file> make_binary_loader_source(config const& cfg, utils::io::file const& file,
                                           std::string_view prefix, std::string_view kind) {
    auto out = BOOST_LEAF_CHECK(io::file::mktemp(".cpp"));
    cfg.begin_task(
        fmt::format("Generate binary loader: {} from {}", out.filename(), file.filename()));

    std::vector<char> buffer;
    auto it = std::back_inserter(buffer);

    fmt::format_to(it, "#include <cstdlib>\n");
    fmt::format_to(it, "#include <stdint.h>\n");
    fmt::format_to(it, "struct bin_info {{ void* ptr; ssize_t len; }};\n");
    fmt::format_to(it, "static struct bin_info bin;\n");
    fmt::format_to(it,
                   "extern \"C\" void __s_add_kernel_registry(char const*, uint32_t, char "
                   "const*, uint32_t, void*, int);\n");
    fmt::format_to(it, "extern \"C\" char {}_start[];\n", prefix);
    fmt::format_to(it, "extern \"C\" uint64_t {}_size;\n", prefix);

    auto const* name = "";
    auto const name_hash = utils::fnv1a(name);
    auto const kind_hash = utils::fnv1a(kind.data(), kind.size());

    fmt::format_to(it, "__attribute__((constructor)) static void do_register() {{\n");
    fmt::format_to(it, "bin.ptr = {}_start; bin.len = {}_size;\n", prefix, prefix);
    fmt::format_to(it,
                   "__s_add_kernel_registry(\"{}\", UINT32_C(0x{:x}), \"{}\", "
                   "UINT32_C(0x{:x}), &bin, 0);\n",
                   name, name_hash, kind, kind_hash);
    fmt::format_to(it, "\n}}\n");

    BOOST_LEAF_CHECK(out.write(0, buffer.data(), buffer.size()));
    BOOST_LEAF_CHECK(save_temps(cfg, out, filetype::other));

    cfg.end_task();

    return out;
}
