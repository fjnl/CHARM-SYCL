#include <boost/assert.hpp>
#include <fmt/format.h>
#include <utils/io.hpp>
#include "config.hpp"
#include "task.hpp"

namespace io = utils::io;
using boost::leaf::result;

result<bool> is_marked_object(io::file const& file) {
    auto const header = BOOST_LEAF_CHECK(file.read(0, 4));
    return header[0] == 'C' && header[1] == 'H' && header[2] == 'S' && header[3] == 'Y';
}

result<std::vector<utils::target>> targets_from_object(io::file const& input_file) {
    if (BOOST_LEAF_CHECK(is_marked_object(input_file)) == false) {
        return {};
    }

    auto const header = BOOST_LEAF_CHECK(input_file.read(0, 8));
    auto const len = static_cast<size_t>(header[4]) | static_cast<size_t>(header[5]) << 8 |
                     static_cast<size_t>(header[6]) << 16 |
                     static_cast<size_t>(header[7]) << 24;
    auto data = BOOST_LEAF_CHECK(input_file.read_str(9, len));

    return utils::from_list_string(data);
}

result<io::file> make_marked_object(config const& cfg, io::file const& input_file,
                                    std::vector<utils::target> targets) {
    auto out = BOOST_LEAF_CHECK(io::file::mktemp(input_file.ext()));

    cfg.begin_task(fmt::format("Generate marked object {} from {}", out.filename(),
                               input_file.filename()));

    std::string t_str;
    for (auto const& t : targets) {
        if (!t_str.empty()) {
            t_str += ',';
        }
        t_str += show(t);
    }

    std::array<char, 8> header;
    header[0] = 'C';
    header[1] = 'H';
    header[2] = 'S';
    header[3] = 'Y';
    header[4] = (t_str.size() >> 0) & 0xff;
    header[5] = (t_str.size() >> 8) & 0xff;
    header[6] = (t_str.size() >> 16) & 0xff;
    header[7] = (t_str.size() >> 24) & 0xff;

    off_t off = 0;
    off = BOOST_LEAF_CHECK(out.write(off, header.data(), header.size()));
    off = BOOST_LEAF_CHECK(out.write(off, "\0", 1));
    off = BOOST_LEAF_CHECK(out.write_str(off, t_str));
    off = BOOST_LEAF_CHECK(out.write(off, "\0", 1));
    off = BOOST_LEAF_CHECK(out.copy_from(off, input_file, 0));

    cfg.end_task();

    return out;
}

result<io::file> extract_from_marked_object(config const& cfg, io::file const& input) {
    BOOST_ASSERT(BOOST_LEAF_CHECK(is_marked_object(input)) == true);

    auto out = BOOST_LEAF_CHECK(io::file::mktemp(input.ext()));

    cfg.begin_task(
        fmt::format("Extract marked object {} from {}", out.filename(), input.filename()));

    auto const header = BOOST_LEAF_CHECK(input.read(0, 8));
    auto const len = static_cast<size_t>(header[4]) | static_cast<size_t>(header[5]) << 8 |
                     static_cast<size_t>(header[6]) << 16 |
                     static_cast<size_t>(header[7]) << 24;

    BOOST_LEAF_CHECK(out.copy_from(0, input, 8 + 1 + len + 1));

    cfg.end_task();

    return out;
}
