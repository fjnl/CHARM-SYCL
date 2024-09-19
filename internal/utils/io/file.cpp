#include <boost/assert.hpp>
#include <fmt/format.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/errors.hpp>
#include <utils/io.hpp>

using boost::leaf::result;

static bool g_disable_removing_tempfiles = false;

namespace utils::io {

file::file(file_desc&& fd, bool need_unlink) : fd_(std::move(fd)), unlink_(need_unlink) {}

file::file(file&& other) = default;

file& file::operator=(file&& other) {
    file tmp(std::move(other));
    swap(tmp);
    return *this;
}

file::~file() {
    close();
}

void file::disable_removing_tempfiles() {
    g_disable_removing_tempfiles = true;
}

void file::swap(file& other) {
    std::swap(fd_, other.fd_);
    std::swap(unlink_, other.unlink_);
}

void file::close() {
    if (unlink_ && fd_ && !g_disable_removing_tempfiles) {
        if (unlink(fd_.filename().c_str()) == -1) {
            fmt::print(stderr, "Cannot remove: {}: {}\n", ::strerror(errno), fd_.filename());
        }
    }
}

result<file> file::create(std::string const& filename) {
    auto fd = BOOST_LEAF_CHECK(file_desc::create(filename));
    return file(std::move(fd), false);
}

result<file> file::readonly(std::string const& filename) {
    auto fd = BOOST_LEAF_CHECK(file_desc::readonly(filename));
    return file(std::move(fd), false);
}

result<file> file::readwrite(std::string const& filename) {
    auto fd = BOOST_LEAF_CHECK(file_desc::readwrite(filename));
    return file(std::move(fd), false);
}

result<file> file::trunc(std::string const& filename) {
    auto fd = BOOST_LEAF_CHECK(file_desc::trunc(filename));
    return file(std::move(fd), false);
}

result<file> file::mktemp() {
    auto fd = BOOST_LEAF_CHECK(file_desc::mktemp());
    return file(std::move(fd), true);
}

result<file> file::mktemp(std::string_view ext) {
    auto fd = BOOST_LEAF_CHECK(file_desc::mktemp(ext));
    return file(std::move(fd), true);
}

result<file> file::mktemp(std::string filename, int extlen) {
    auto fd = BOOST_LEAF_CHECK(file_desc::mktemp(filename, extlen));
    return file(std::move(fd), true);
}

int file::fd() const {
    return fd_.get();
}

std::string const& file::filename() const {
    return fd_.filename();
}

std::string_view file::ext() const {
    if (auto const p = filename().rfind('.'); p != std::string::npos) {
        return std::string_view(filename().cbegin() + p, filename().cend());
    }
    return {};
}

std::string_view file::stem() const {
    std::string_view fn = filename();

    BOOST_ASSERT(!fn.ends_with("/"));
    if (fn.ends_with("/")) {
        std::abort();
    }

    if (auto const p = fn.rfind('/'); p != std::string::npos) {
        fn = fn.substr(p + 1);
    }

    if (auto const q = fn.rfind('.'); q != std::string::npos) {
        fn = fn.substr(0, q);
    }

    return fn;
}

boost::leaf::result<struct ::stat> file::stat() const {
    struct stat st;
    if (fstat(fd(), &st) == -1) {
        return make_io_error("fstat", filename());
    }
    return st;
}

result<void> file::reopen() {
    return fd_.reopen();
}

result<std::vector<char>> file::read_all() const {
    std::vector<char> buffer;
    size_t len = 0;
    off_t off = 0;

    for (;;) {
        if (buffer.size() < len + 512) {
            buffer.resize(buffer.size() + 512);
        }

        auto const nr = BOOST_LEAF_CHECK(readsome(off, buffer.data() + len, 512));

        if (nr == 0) {
            buffer.resize(len);
            break;
        }

        len += nr;
        off += nr;
    }

    return buffer;
}

result<std::string> file::read_all_str() const {
    auto res = BOOST_LEAF_CHECK(read_all());
    return std::string(res.begin(), res.end());
}

result<std::vector<char>> file::read(off_t off, size_t len) const {
    auto res = BOOST_LEAF_CHECK(readsome(off, len));
    if (res.size() != len) {
        return make_io_error(unexpected_eof{}, "pread", filename());
    }
    return res;
}

result<size_t> file::readsome(off_t off, void* ptr, size_t maxlen) const {
    auto loader = boost::leaf::on_error(boost::leaf::e_file_name{filename()});

    auto const nr = BOOST_LEAF_CHECK(io::pread(fd(), ptr, maxlen, off));

    return nr;
}

result<std::vector<char>> file::readsome(off_t off, size_t maxlen) const {
    std::vector<char> res(maxlen);
    auto const nr = BOOST_LEAF_CHECK(readsome(off, res.data(), res.size()));
    res.resize(nr);
    return res;
}

result<std::string> file::read_str(off_t off, size_t len) const {
    auto vec = BOOST_LEAF_CHECK(read(off, len));
    return std::string(vec.begin(), vec.end());
}

result<std::string> file::readsome_str(off_t off, size_t maxlen) const {
    auto vec = BOOST_LEAF_CHECK(readsome(off, maxlen));
    return std::string(vec.begin(), vec.end());
}

result<off_t> file::write(off_t off, void const* ptr, size_t len) {
    auto loader = boost::leaf::on_error(boost::leaf::e_file_name{filename()});

    auto nw = BOOST_LEAF_CHECK(io::pwrite(fd(), ptr, len, off));

    if (nw != len) {
        return make_io_error(partial_write{}, "pwrite");
    }

    return off + nw;
}

result<off_t> file::write_str(off_t off, std::string const& data) {
    return write(off, data.data(), data.size());
}

result<off_t> file::copy_from(off_t off, file const& src, off_t src_off) {
    std::vector<char> buff(4096);

    for (;;) {
        auto const nr = BOOST_LEAF_CHECK(src.readsome(src_off, buff.data(), buff.size()));

        if (nr == 0) {
            break;
        }

        BOOST_LEAF_CHECK(write(off, buff.data(), nr));

        src_off += nr;
        off += nr;
    }

    return off;
}

}  // namespace utils::io
