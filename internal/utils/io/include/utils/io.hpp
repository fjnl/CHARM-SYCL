#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <vector>
#include <boost/leaf.hpp>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/errors.hpp>

namespace utils::io {

struct file_desc;

struct io_error {};

template <class T>
concept IoError = std::derived_from<T, io_error>;

struct partial_write : io_error {};

struct unexpected_eof : io_error {};

struct subprocess_error : io_error {
    bool signaled;
    int signal;
    int exitstatus;

    static subprocess_error make_signaled(int s) {
        subprocess_error err;
        err.signaled = true;
        err.signal = s;
        err.exitstatus = -1;
        return err;
    }

    static subprocess_error make_exited(int s) {
        subprocess_error err;
        err.signaled = false;
        err.signal = -1;
        err.exitstatus = s;
        return err;
    }
};

[[noreturn]] void report_and_exit(char const* argv0, io_error const& err,
                                  boost::leaf::e_api_function const* fn,
                                  boost::leaf::e_file_name const* file,
                                  utils::errors::e_backtrace const* bt);

[[noreturn]] void report_and_exit(char const* argv0, partial_write const& err,
                                  boost::leaf::e_api_function const* fn,
                                  boost::leaf::e_file_name const* file,
                                  utils::errors::e_backtrace const* bt);

[[noreturn]] void report_and_exit(char const* argv0, unexpected_eof const& err,
                                  boost::leaf::e_api_function const* fn,
                                  boost::leaf::e_file_name const* file,
                                  utils::errors::e_backtrace const* bt);

[[noreturn]] void report_and_exit(char const* argv0, subprocess_error const& err,
                                  boost::leaf::e_api_function const* fn,
                                  boost::leaf::e_file_name const* file,
                                  utils::errors::e_backtrace const* bt);

template <class F>
auto try_handle_some(char const* argv0, F&& f) {
    using result_type = std::decay_t<decltype(f().value())>;

    return boost::leaf::try_handle_some(
        std::forward<F>(f),
        [&](partial_write const& err, boost::leaf::e_api_function const* fn,
            boost::leaf::e_file_name const* file,
            utils::errors::e_backtrace const* bt) -> result_type {
            report_and_exit(argv0, err, fn, file, bt);
        },
        [&](unexpected_eof const& err, boost::leaf::e_api_function const* fn,
            boost::leaf::e_file_name const* file,
            utils::errors::e_backtrace const* bt) -> result_type {
            report_and_exit(argv0, err, fn, file, bt);
        },
        [&](subprocess_error const& err, boost::leaf::e_api_function const* fn,
            boost::leaf::e_file_name const* file,
            utils::errors::e_backtrace const* bt) -> result_type {
            report_and_exit(argv0, err, fn, file, bt);
        },
        [&](io_error const& err, boost::leaf::e_api_function const* fn,
            boost::leaf::e_file_name const* file,
            utils::errors::e_backtrace const* bt) -> result_type {
            report_and_exit(argv0, err, fn, file, bt);
        });
}

template <class... Items>
auto make_io_error(IoError auto const& err, Items&&... items) {
    return boost::leaf::new_error(
        err,
        [](utils::errors::e_backtrace& e) {
            utils::errors::load_backtrace(e);
        },
        std::forward<Items>(items)...);
}

template <class... Items>
auto make_io_error(IoError auto const& err, char const* fn, Items&&... items) {
    return boost::leaf::new_error(err, boost::leaf::e_api_function{fn},
                                  std::forward<Items>(items)...);
}

template <class... Items>
auto make_io_error(IoError auto const& err, char const* fn, std::string const& filename,
                   Items&&... items) {
    return make_io_error(err, fn, boost::leaf::e_file_name{filename},
                         std::forward<Items>(items)...);
}

template <class... Items>
auto make_io_error(char const* fn, Items&&... items) {
    return boost::leaf::new_error(
        boost::leaf::e_errno{}, boost::leaf::e_api_function{fn},
        [](utils::errors::e_backtrace& e) {
            utils::errors::load_backtrace(e);
        },
        std::forward<Items>(items)...);
}

template <class... Items>
auto make_io_error(char const* fn, std::string const& filename, Items&&... items) {
    return make_io_error(fn, boost::leaf::e_file_name{filename}, std::forward<Items>(items)...);
}

[[nodiscard]] boost::leaf::result<size_t> pread(int fd, void* buf, size_t len, off_t off);

[[nodiscard]] boost::leaf::result<size_t> pwrite(int fd, void const* buf, size_t len,
                                                 off_t off);

[[nodiscard]] boost::leaf::result<int> open(char const* filename, int flags,
                                            mode_t mode = 0644);

[[nodiscard]] boost::leaf::result<int> open(std::string const& filename, int flags,
                                            mode_t mode = 0644);

struct exec_opts {
    static int constexpr devnull = -1;

    int stdin = STDIN_FILENO;
    int stdout = STDOUT_FILENO;
    int stderr = STDERR_FILENO;
    mode_t umask = 0077;
    bool check = true;
};

[[nodiscard]] boost::leaf::result<bool> fork_and_execv(std::string const& path,
                                                       std::vector<std::string> const& args);

[[nodiscard]] boost::leaf::result<bool> fork_and_execv(std::string const& path,
                                                       std::vector<std::string> const& args,
                                                       exec_opts const& opts);

[[nodiscard]] boost::leaf::result<std::string> read_proc_self_exe();

struct file_desc {
    file_desc(file_desc const&) = delete;

    file_desc(file_desc&& other);

    file_desc& operator=(file_desc const&) = delete;

    file_desc& operator=(file_desc&&);

    ~file_desc();

    explicit operator bool() const;

    int get() const;

    std::string const& filename() const;

    int release();

    [[nodiscard]] boost::leaf::result<void> reopen();

    [[nodiscard]] static boost::leaf::result<file_desc> create(std::string const& filename);

    [[nodiscard]] static boost::leaf::result<file_desc> readonly(std::string const& filename);

    [[nodiscard]] static boost::leaf::result<file_desc> readwrite(std::string const& filename);

    [[nodiscard]] static boost::leaf::result<file_desc> trunc(std::string const& filename);

    [[nodiscard]] static boost::leaf::result<file_desc> mktemp();

    [[nodiscard]] static boost::leaf::result<file_desc> mktemp(std::string_view ext);

    [[nodiscard]] static boost::leaf::result<file_desc> mktemp(std::string filename,
                                                               int extlen);

private:
    file_desc();

    struct impl;
    std::unique_ptr<impl> pimpl_;
};

struct file {
    explicit file(file_desc&& fd, bool need_unlink = false);

    file(file const&) = delete;

    file(file&& other);

    file& operator=(file const&) = delete;

    file& operator=(file&& other);

    ~file();

    [[nodiscard]] static boost::leaf::result<file> create(std::string const& filename);

    [[nodiscard]] static boost::leaf::result<file> readonly(std::string const& filename);

    [[nodiscard]] static boost::leaf::result<file> readwrite(std::string const& filename);

    [[nodiscard]] static boost::leaf::result<file> trunc(std::string const& filename);

    [[nodiscard]] static boost::leaf::result<file> mktemp();

    [[nodiscard]] static boost::leaf::result<file> mktemp(std::string_view ext);

    [[nodiscard]] static boost::leaf::result<file> mktemp(std::string filename, int extlen);

    static void disable_removing_tempfiles();

    std::string const& filename() const;

    std::string_view ext() const;

    std::string_view stem() const;

    int fd() const;

    [[nodiscard]] boost::leaf::result<struct ::stat> stat() const;

    [[nodiscard]] boost::leaf::result<std::vector<char>> read_all() const;

    [[nodiscard]] boost::leaf::result<std::string> read_all_str() const;

    [[nodiscard]] boost::leaf::result<std::vector<char>> read(off_t off, size_t len) const;

    [[nodiscard]] boost::leaf::result<std::vector<char>> readsome(off_t off,
                                                                  size_t maxlen) const;

    [[nodiscard]] boost::leaf::result<size_t> readsome(off_t off, void* ptr,
                                                       size_t maxlen) const;

    [[nodiscard]] boost::leaf::result<std::string> read_str(off_t off, size_t len) const;

    [[nodiscard]] boost::leaf::result<std::string> readsome_str(off_t off, size_t maxlen) const;

    [[nodiscard]] boost::leaf::result<off_t> write(off_t off, void const* ptr, size_t len);

    [[nodiscard]] boost::leaf::result<off_t> write_str(off_t off, std::string const& data);

    [[nodiscard]] boost::leaf::result<off_t> copy_from(off_t off, file const& src,
                                                       off_t src_off);

    [[nodiscard]] boost::leaf::result<off_t> copy_from(file const& src) {
        return copy_from(0, src, 0);
    }

    [[nodiscard]] boost::leaf::result<void> reopen();

    void swap(file& other);

private:
    void close();

    file_desc fd_;
    bool unlink_;
};

}  // namespace utils::io
