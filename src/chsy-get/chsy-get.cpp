#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <system_error>
#include <vector>
#include <cxxopts.hpp>
#include <fmt/format.h>
#include <xcml.hpp>
#include <xcml_recusive_visitor.hpp>

namespace {

std::vector<char> out_buffer;
auto out = std::back_inserter(out_buffer);

[[noreturn]] void throw_errno(std::string_view errmsg, int err,
                              char const* file = __builtin_FILE(),
                              int line = __builtin_LINE()) {
    auto const what = fmt::format("Error at {}:{}: {}", file, line, errmsg);
    throw std::system_error(std::error_code(err, std::generic_category()), what);
}

void write_file(FILE* fp, std::string_view filename, std::vector<char> const& data,
                bool close) {
    if (!fp) {
        throw_errno(fmt::format("fopen: {}", filename), errno);
    }
    if (fwrite(data.data(), data.size(), 1, fp) != 1) {
        auto const err = errno;
        if (close) {
            fclose(fp);
        }
        throw_errno(fmt::format("fwrite: {}", filename), err);
    }
    if (fflush(fp)) {
        auto const err = errno;
        if (close) {
            fclose(fp);
        }
        throw_errno(fmt::format("fflush: {}", filename), err);
    }
    if (close) {
        if (fclose(fp)) {
            throw_errno(fmt::format("fclose: {}", filename), errno);
        }
    }
}

struct get_kernels_visitor final : xcml::recursive_visitor<get_kernels_visitor> {
    std::vector<std::string> res;

    xcml::node_ptr visit_kernel_wrapper_decl(xcml::kernel_wrapper_decl_ptr const& node,
                                             scope_ref) {
        res.push_back(node->name);
        return node;
    }
};

void get_kernels(xcml::xcml_program_node_ptr prg) {
    get_kernels_visitor vis;
    vis.apply(prg);

    for (auto const& name : vis.res) {
        fmt::format_to(out, "{}\n", name);
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        cxxopts::Options options(argv[0]);
        options.add_options()("input", "input file",
                              cxxopts::value<std::string>()->default_value("-"));
        options.add_options()("o,output", "output file",
                              cxxopts::value<std::string>()->default_value("-"));
        options.add_options()("kernels", "generate kernel list", cxxopts::value<bool>());
        options.parse_positional("input");
        auto opts = options.parse(argc, argv);

        auto const& input = opts["input"].as<std::string>();
        auto const& output = opts["output"].as<std::string>();
        auto const& mode_kernels = opts["kernels"].as<bool>();

        auto prg = xcml::xml_to_prg(xcml::read_xml(input));

        if (mode_kernels) {
            get_kernels(prg);
        } else {
            fmt::print(stderr, "{}: No mode options are given.\n", argv[0]);
            return 1;
        }

        if (output == "-") {
            write_file(stdout, "<stdout>", out_buffer, false);
        } else {
            write_file(fopen(output.c_str(), "w"), output, out_buffer, true);
        }
    } catch (std::exception const& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
