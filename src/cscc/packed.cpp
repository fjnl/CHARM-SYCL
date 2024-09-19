#include <array>
#include <vector>
#include <utils/io.hpp>
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

constexpr auto HEADER_LEN = 4;

result<void> link_packed_file(config const& cfg, std::vector<io::file> const& inputs) {
    cfg.begin_task("Generating packed object");

    auto output = BOOST_LEAF_CHECK(io::file::create(cfg.output));
    std::array<char, HEADER_LEN> header = {{'C', 'H', 'S', 'P'}};
    off_t off = header.size() + sizeof(size_t) + sizeof(size_t) * inputs.size();
    std::vector<size_t> len_array;

    for (auto const& in : inputs) {
        auto const new_off = BOOST_LEAF_CHECK(output.copy_from(off, in, 0));

        cfg.task_msg(
            fmt::format("added {} (off={}, size={})", in.filename(), off, new_off - off));

        len_array.push_back(new_off - off);
        off = new_off;
    }

    auto const n_files = len_array.size();
    off = 0;

    off = BOOST_LEAF_CHECK(output.write(off, header.data(), header.size()));
    off = BOOST_LEAF_CHECK(output.write(off, &n_files, sizeof(n_files)));
    off = BOOST_LEAF_CHECK(
        output.write(off, len_array.data(), len_array.size() * sizeof(size_t)));

    cfg.end_task();
    return {};
}

result<bool> is_packed_file(io::file const& file) {
    auto const header = BOOST_LEAF_CHECK(file.read(0, 4));
    return header[0] == 'C' && header[1] == 'H' && header[2] == 'S' && header[3] == 'P';
}

result<std::vector<io::file>> extract_packed_file(config const& cfg, io::file const& input) {
    cfg.begin_task("Extracting packed object");
    cfg.task_msg(fmt::format("from {}", input.filename()));

    size_t n_files;
    off_t off = HEADER_LEN;

    auto const n_data = BOOST_LEAF_CHECK(input.read(off, sizeof(n_files)));
    memcpy(&n_files, n_data.data(), sizeof(n_files));
    off += sizeof(n_files);

    std::vector<size_t> len_array(n_files);
    auto const buffer = BOOST_LEAF_CHECK(input.read(off, n_files * sizeof(size_t)));
    memcpy(len_array.data(), buffer.data(), n_files * sizeof(size_t));
    off += n_files * sizeof(size_t);

    std::vector<io::file> files;

    for (auto const len : len_array) {
        auto f = BOOST_LEAF_CHECK(io::file::mktemp(".o"));

        auto const data = BOOST_LEAF_CHECK(input.read(off, len));
        BOOST_LEAF_CHECK(f.write(0, data.data(), data.size()));

        cfg.task_msg(
            fmt::format("extracted into {} (off={}, size={})", f.filename(), off, len));
        off += len;

        files.push_back(std::move(f));
    }

    cfg.end_task();

    return files;
}
