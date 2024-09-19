#include <boost/assert.hpp>
#include <utils/io.hpp>
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

namespace {

std::string kernel_ext(utils::target t) {
    using enum utils::target;

    switch (t) {
        case NONE:
            break;
        case CPU_C:
        case CPU_OPENMP:
            return ".c";
        case NVIDIA_CUDA:
            return ".cu";
        case AMD_HIP:
            return ".cpp";
    }

    std::terminate();
}

[[nodiscard]] result<void> collect_symbols(config const& cfg, io::file const& input,
                                           std::vector<std::string>& symbols) {
    cfg.begin_task("Collect Kernel Symbols");

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(".txt"));

    BOOST_LEAF_CHECK(run_self(
        cfg, {"__chsy_get__", "--kernels", "--output", out.filename(), input.filename()}));
    BOOST_LEAF_CHECK(save_temps(cfg, out, filetype::other));

    auto const list = BOOST_LEAF_CHECK(out.read_all_str());

    if (list.empty()) {
        return {};
    }

    for (size_t pos = 0; pos < list.size();) {
        auto const nl = list.find("\n", pos);

        auto const end = nl == std::string::npos ? list.size() : nl;
        auto const name = list.substr(pos, end - pos);

        if (!name.empty()) {
            symbols.push_back(name);
        }

        pos = end + 1;
    }

    cfg.end_task();

    return {};
}

}  // namespace

result<io::file> run_clang_format(config const& cfg, io::file const& input) {
    auto cmd = cfg.clang_format();

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(input.ext()));

    if (cmd) {
        cfg.begin_task("Apply clang-format");

        io::exec_opts opt;
        opt.stdout = out.fd();
        BOOST_LEAF_CHECK(run_command(cfg, {*cmd, "--style=Chromium", input.filename()}, opt));

        cfg.end_task();
    } else {
        BOOST_LEAF_CHECK(out.copy_from(0, input, 0));
    }

    return out;
}

[[nodiscard]] result<std::pair<file_map, io::file>> run_kext(
    config const& cfg, io::file const& input,
    [[maybe_unused]] std::vector<std::string>& symbols) {
    file_map outs;

    cfg.begin_task("Extract Kernels");

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(".xml"));
    auto desc = BOOST_LEAF_CHECK(io::file::mktemp(".hpp"));

    BOOST_LEAF_CHECK(
        run_self(cfg, {"__chsy_kext__", "--output", out.filename(), "--desc", desc.filename(),
                       input.filename(), "--", "-Xclang", "-fsycl-is-device", "-std=c++20"}));
    BOOST_LEAF_CHECK(save_temps(cfg, out, filetype::xml));

    cfg.end_task();

    auto const st = BOOST_LEAF_CHECK(out.stat());
    if (st.st_size > 0) {
        bool symbols_collected = false;

        for (auto const t : cfg.targets) {
            auto out2 = BOOST_LEAF_CHECK(io::file::mktemp(".xml"));
            BOOST_LEAF_CHECK(out2.copy_from(out));

            if (!symbols_collected && is_cpu(t)) {
                BOOST_LEAF_CHECK(collect_symbols(cfg, out2, symbols));
                symbols_collected = true;
            }

            outs.emplace(t, std::move(out2));
        }
    }

    return std::make_pair(std::move(outs), std::move(desc));
}

result<file_map> run_lower(config const& cfg, file_map const& input_files) {
    cfg.begin_task("Lowering Kernels");

    file_map outs;

    for (auto const& [target, input] : input_files) {
        cfg.begin_task(fmt::format("for {}", show(target)));

        auto out = BOOST_LEAF_CHECK(io::file::mktemp(".xml"));

        BOOST_LEAF_CHECK(run_self(cfg, {"__chsy_lower__", "--output", out.filename(),
                                        "--target", show(target), input.filename()}));

        BOOST_LEAF_CHECK(save_temps(cfg, out, target, filetype::xml));

        cfg.end_task();

        outs.emplace(target, std::move(out));
    }

    cfg.end_task();

    return outs;
}

result<file_map> run_cback(config const& cfg, file_map const& input_files) {
    cfg.begin_task("Generating Kernel Code");
    file_map outs;

    for (auto const& [target, input] : input_files) {
        cfg.begin_task(fmt::format("for {}", show(target)));

        auto out = BOOST_LEAF_CHECK(io::file::mktemp(kernel_ext(target).c_str()));

        BOOST_LEAF_CHECK(run_self(cfg, {"__chsy_cback__", "--output", out.filename(),
                                        "--target", show(target), input.filename()}));

        out = BOOST_LEAF_CHECK(run_clang_format(cfg, out));

        BOOST_LEAF_CHECK(save_temps(cfg, out, target, filetype::kernel));

        cfg.end_task();

        outs.emplace(target, std::move(out));
    }

    cfg.end_task();

    return outs;
}

result<io::file> bin2asm(config const& cfg, io::file const& input, std::string& prefix) {
    cfg.begin_task("Generating Asm");

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(".s"));
    prefix = out.stem();

    BOOST_LEAF_CHECK(
        run_self(cfg, {"__chsy_bin2asm__", input.filename(), prefix, out.filename()}));

    cfg.end_task();

    return out;
}
