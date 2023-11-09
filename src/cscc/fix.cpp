#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <system_error>
#include <fmt/format.h>

namespace {

[[noreturn]] void throw_errno(std::string_view errmsg, int err,
                              char const* file = __builtin_FILE(),
                              int line = __builtin_LINE()) {
    auto const what = fmt::format("Error at {}:{}: {}", file, line, errmsg);
    throw std::system_error(std::error_code(err, std::generic_category()), what);
}

void write_file(FILE* fp, std::string_view filename, std::string const& data) {
    if (!fp) {
        throw_errno(fmt::format("fopen: {}", filename), errno);
    }
    if (fwrite(data.data(), data.size(), 1, fp) != 1) {
        auto const err = errno;
        fclose(fp);
        throw_errno(fmt::format("fwrite: {}", filename), err);
    }
    if (fflush(fp)) {
        auto const err = errno;
        fclose(fp);
        throw_errno(fmt::format("fflush: {}", filename), err);
    }
    if (fclose(fp)) {
        throw_errno(fmt::format("fclose: {}", filename), errno);
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::string out;
        auto const output = argv[1];
        auto const binary_dir = std::filesystem::path(argv[2]);

        out += "#pragma once\n";

        for (int i = 3; i < argc; i += 2) {
            auto const name = argv[i];
            auto path = std::filesystem::path(argv[i + 1]);

            auto it1 = binary_dir.begin();
            auto const end1 = binary_dir.end();
            auto it2 = path.begin();
            auto const end2 = path.end();

            for (;;) {
                if (it1 == end1 || *it1 != *it2) {
                    std::filesystem::path tmp;
                    for (; it2 != end2; ++it2) {
                        tmp /= *it2;
                    }
                    path = tmp;
                    break;
                }
                if (it2 == end2) {
                    break;
                }

                ++it1;
                ++it2;
            }

            out += fmt::format("#define {} \"{}\"\n", name, path.string());
        }

        write_file(fopen(output, "w"), output, out);
    } catch (std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
