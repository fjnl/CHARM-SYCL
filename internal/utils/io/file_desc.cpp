#include <fcntl.h>
#include <fmt/format.h>
#include <unistd.h>
#include <utils/errors.hpp>
#include <utils/io.hpp>

using boost::leaf::result;
namespace leaf = boost::leaf;
namespace errs = utils::errors;

namespace utils::io {

struct file_desc::impl {
    int fd_ = -1;
    int flags_ = 0;
    std::string filename_;

    impl() = default;

    impl(impl const&) = delete;

    impl(impl&&) = delete;

    impl& operator=(impl const&) = delete;

    impl& operator=(impl&&) = delete;

    ~impl() {
        auto fd = std::exchange(fd_, -1);
        if (fd >= 0) {
            if (::close(fd)) {
                fmt::print(stderr, "Failed to close: {}: {}\n", strerror(errno), filename_);
            }
        }
    }

    [[nodiscard]] result<void> open_(std::string const& filename, int flags, mode_t mode) {
        flags_ = flags;
        fd_ = BOOST_LEAF_CHECK(io::open(filename, flags_, mode));
        filename_ = filename;
        return {};
    }

    [[nodiscard]] result<void> create_(std::string const& filename) {
        return open_(filename, O_CREAT | O_RDWR | O_CLOEXEC | O_TRUNC, 0644);
    }

    [[nodiscard]] result<void> readwrite_(std::string const& filename) {
        return open_(filename, O_RDWR | O_CLOEXEC, 0644);
    }

    [[nodiscard]] result<void> trunc_(std::string const& filename) {
        return open_(filename, O_RDWR | O_CLOEXEC | O_TRUNC, 0644);
    }

    [[nodiscard]] result<void> readonly_(std::string const& filename) {
        return open_(filename, O_RDONLY | O_CLOEXEC, 0644);
    }
};

file_desc::file_desc() : pimpl_(std::make_unique<impl>()) {}

file_desc::~file_desc() = default;

file_desc::file_desc(file_desc&& other) = default;

file_desc& file_desc::operator=(file_desc&&) = default;

file_desc::operator bool() const {
    return pimpl_ && pimpl_->fd_ >= 0;
}

int file_desc::get() const {
    return pimpl_->fd_;
}

std::string const& file_desc::filename() const {
    return pimpl_->filename_;
}

int file_desc::release() {
    return std::exchange(pimpl_->fd_, -1);
}

result<file_desc> file_desc::create(std::string const& filename) {
    file_desc fd;
    BOOST_LEAF_CHECK(fd.pimpl_->create_(filename));
    return fd;
}

result<file_desc> file_desc::readonly(std::string const& filename) {
    file_desc fd;
    BOOST_LEAF_CHECK(fd.pimpl_->readonly_(filename));
    return fd;
}

result<file_desc> file_desc::readwrite(std::string const& filename) {
    file_desc fd;
    BOOST_LEAF_CHECK(fd.pimpl_->readwrite_(filename));
    return fd;
}

result<file_desc> file_desc::trunc(std::string const& filename) {
    file_desc fd;
    BOOST_LEAF_CHECK(fd.pimpl_->trunc_(filename));
    return fd;
}

boost::leaf::result<file_desc> file_desc::mktemp() {
    return mktemp({});
}

boost::leaf::result<file_desc> file_desc::mktemp(std::string_view ext) {
    std::string templ;

    if (auto const* tmpdir = getenv("TMPDIR")) {
        templ = std::string(tmpdir);
        if (!templ.ends_with("/")) {
            templ += '/';
        }
        templ += "tmpXXXXXX";
    } else {
        templ = "/tmp/tmpXXXXXX";
    }

    templ += ext;

    return mktemp(templ.c_str(), ext.size());
}

boost::leaf::result<file_desc> file_desc::mktemp(std::string filename, int extlen) {
    file_desc fd;

    fd.pimpl_->fd_ = mkostemps(const_cast<char*>(filename.data()), extlen, O_CLOEXEC);

    if (fd.pimpl_->fd_ == -1) {
        return errs::make_system_error("mkostemps", leaf::e_file_name{filename});
    }

    fd.pimpl_->filename_ = std::move(filename);

    return fd;
}

boost::leaf::result<void> file_desc::reopen() {
    return pimpl_->open_(pimpl_->filename_, pimpl_->flags_, 0644);
}

}  // namespace utils::io
