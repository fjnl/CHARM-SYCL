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

struct info_desc {
    std::string type;
    std::string name;

    explicit operator bool() const {
        return !type.empty();
    }
};

static info_desc const PLATFORM_INFOS[] = {
    {"std::string", "version"},
    {"std::string", "name"},
    {"std::string", "vendor"},
    {},
};

static info_desc const DEVICE_INFOS[] = {
    {"std::string", "name"},
    {"std::string", "vendor"},
    {"std::string", "driver_version"},
    {},
};

void gen_decl(info_desc const* descs, std::string_view ns) {
    pr("namespace info {{");
    pr("namespace {} {{", ns);
    sep();

    for (auto d = descs; *d; ++d) {
        pr("struct {} {{", d->name);
        pr("using return_type = {};", d->type);
        pr("}};");
        sep();
    }

    pr("}} }}");
}

void gen_impl(info_desc const* descs, std::string_view klass, std::string_view ns) {
    pr("template <class Param>");
    pr("typename Param::return_type {}::get_info() const {{", klass);

    for (auto d = descs; *d; ++d) {
        if (d == descs) {
            pr("if constexpr");
        } else {
            pr("else if constexpr");
        }

        pr("(std::is_same_v<Param, info::{}::{}>)", ns, d->name);
        pr("{{ return impl_->info_{}(); }}", d->name);
    }

    pr("else {{ static_assert(not_supported<Param>, \"invalid info param type\"); }}");

    pr("}}");
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

    if (mode == "platform-decl") {
        gen_decl(PLATFORM_INFOS, "platform");
    } else if (mode == "platform-impl") {
        gen_impl(PLATFORM_INFOS, "platform", "platform");
    } else if (mode == "device-decl") {
        gen_decl(DEVICE_INFOS, "device");
    } else if (mode == "device-impl") {
        gen_impl(DEVICE_INFOS, "device", "device");
    } else {
        fmt::print(stderr, "Error: unknown: {}\n", mode);
        return 1;
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
