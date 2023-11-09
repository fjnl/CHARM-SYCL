#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fcntl.h>
#include <fmt/compile.h>
#include <fmt/format.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utils/hash.hpp>
#include <utils/target.hpp>

std::filesystem::path cscc_tool_path(char const* dir, char const* name);
std::filesystem::path cscc_include_path_bin();
std::filesystem::path cscc_include_path_src();
std::filesystem::path cscc_sycl_runtime_library();
#ifdef USE_IRIS
std::filesystem::path cscc_iris_omp_loader();
#endif

namespace u = utils;

namespace {

struct exit_failure {
    explicit exit_failure(int s = EXIT_FAILURE) : status(s) {}

    int status;
};

[[noreturn]] void throw_errno_impl(char const* errmsg, int errno_,
                                   char const* file = __builtin_FILE(),
                                   int line = __builtin_LINE()) {
    auto const what = fmt::format("Errot at {}:{}: {}", file, line, errmsg);
    throw std::system_error(std::error_code(errno_, std::generic_category()), what);
}

#define throw_errno(errmsg)                  \
    ({                                       \
        /* save errno immediately */         \
        auto const errno__ = errno;          \
        throw_errno_impl((errmsg), errno__); \
    })

#define throw_errno_fmt(fmt_str, ...)                                                      \
    ({                                                                                     \
        /* save errno immediately */                                                       \
        auto const errno__ = errno;                                                        \
        throw_errno_impl(fmt::format(FMT_COMPILE(fmt_str), __VA_ARGS__).c_str(), errno__); \
    })

bool has_prefix(std::string_view large, std::string_view small) {
    if (large.size() >= small.size()) {
        return large.substr(0, small.size()) == small;
    }
    return false;
}

bool has_suffix(std::string_view large, std::string_view small) {
    if (large.size() >= small.size()) {
        return large.substr(large.size() - small.size(), large.size()) == small;
    }
    return false;
}

std::string_view trim_ext(std::string_view filename) {
    if (auto const pos = filename.rfind('.'); pos != std::string_view::npos) {
        return filename.substr(0, pos);
    }
    return filename;
}

std::string_view get_ext(std::string_view filename) {
    if (auto const pos = filename.rfind('.'); pos != std::string_view::npos) {
        return filename.substr(pos);
    }
    throw std::runtime_error("No extentions found in the path");
}

bool has_ext(std::string_view filename, std::string_view ext) {
    return get_ext(filename) == ext;
}

#ifndef CMAKE_C_COMPILER
#    define CMAKE_C_COMPILER "cc"
#endif

#ifndef CMAKE_CXX_COMPILER
#    define CMAKE_CXX_COMPILER "c++"
#endif

#ifndef CMAKE_CUDA_COMPILER
#    define CMAKE_CUDA_COMPILER "nvcc"
#endif

#ifndef CMAKE_CUDA_HOST_COMPILER
#    define CMAKE_CUDA_HOST_COMPILER ""
#endif

#ifndef CMAKE_AR
#    define CMAKE_AR "ar"
#endif

#ifndef OBJCOPY_COMMAND
#    define OBJCOPY_COMMAND "objcopy"
#endif

#ifndef OBJDUMP_COMMAND
#    define OBJDUMP_COMMAND "objdump"
#endif

#ifndef NM_COMMAND
#    define NM_COMMAND "nm"
#endif

#ifndef CLANG_FORMAT_COMMAND
#    define CLANG_FORMAT_COMMAND ""
#endif

#ifdef CUDAToolkit
#    ifndef CUDA_LIBRARY
#        define CUDA_LIBRARY "-lcuda"
#    endif
#endif

#ifdef hip_FOUND
#    ifndef HIP_HIPCC_EXECUTABLE
#        define HIP_HIPCC_EXECUTABLE "hipcc"
#    endif
#endif

enum class filetype { kernel, xml, other };

struct config {
    std::vector<std::string> inputs;
    std::vector<u::target> targets;
    std::string output;
    std::string stop_at;
    bool debug = false;
    bool verbose = false;
    bool save_temps = false;
    bool save_kernels = false;
    bool save_xmls = false;
    bool do_link = true;

    std::string tool_path(char const* dir, char const* name) const {
        return cscc_tool_path(dir, name);
    }

    std::string include_path_bin() const {
        return cscc_include_path_bin();
    }

    std::string include_path_src() const {
        return cscc_include_path_src();
    }

    std::filesystem::path sycl_runtime_library() const {
        return cscc_sycl_runtime_library();
    }

#ifdef USE_IRIS
    std::filesystem::path iris_omp_loader() const {
        return cscc_iris_omp_loader();
    }
#endif

    std::string cc = CMAKE_C_COMPILER;
    std::string cxx = CMAKE_CXX_COMPILER;
    std::string ccbin = CMAKE_CUDA_HOST_COMPILER;
    std::string ar = CMAKE_AR;
    std::string linker;
    std::string k_cc;
    std::string objcopy = OBJCOPY_COMMAND;
    std::string objdump = OBJDUMP_COMMAND;
    std::string nm = NM_COMMAND;
    std::string clang_format = CLANG_FORMAT_COMMAND;

    std::string const& cc_for_kernel() const {
        return k_cc.empty() ? cc : k_cc;
    }

    char opt_level = '0';
    std::vector<std::string> remainder;
    std::unordered_map<std::string, std::string> defines;
    std::vector<std::string> include_dirs;
    std::vector<std::string> libraries;
    std::vector<std::string> library_dirs;
    std::string cxx_std = "c++17";
    bool md = false, mmd = false, mm = false, m = false;
    std::string mf, mt;

#ifdef CUDAToolkit
    std::string nvcc = CMAKE_CUDA_COMPILER;
    std::string cuda_library_path = CUDA_LIBRARY;
    std::string cuda_arch;
#endif

#ifdef hip_FOUND
    std::string hipcc = HIP_HIPCC_EXECUTABLE;
#endif

#ifdef CHARM_SYCL_ENABLE_ASAN
    std::string fsanitize = "address";
#else
    std::string fsanitize;
#endif

    void validate() {
        if (inputs.empty()) {
            fmt::print(stderr, FMT_COMPILE("Error: No input file\n"));
            throw exit_failure();
        }

        if (do_link) {
            if (output.empty()) {
                output = "a.out";
            }
        } else {
            if (inputs.size() > 1) {
                fmt::print(stderr, FMT_COMPILE("Error: too many inputs: {}\n"), inputs.size());
                throw exit_failure();
            }
            if (output.empty()) {
                output.append(trim_ext(inputs.front()));
                output += ".a";
            }
        }

        if (linker.empty()) {
            linker = cxx;
        }

        if (!do_link) {
            if (!has_suffix(output, ".a")) {
                fmt::print(stderr, FMT_COMPILE("Error: -o must end with '.a'\n"));
                throw exit_failure();
            }
        }

        if (targets.empty()) {
            targets = utils::default_targets();
        }
    }
};

std::string_view sv(char const* cstr) {
    return {cstr};
}

template <class Timepoint>
double delta_sec(Timepoint const& begin, Timepoint const& end) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1.0e9;
}

struct file_descriptor {
    file_descriptor() = default;

    explicit file_descriptor(int fd) : fd_(fd) {}

    file_descriptor(file_descriptor const&) = delete;

    file_descriptor(file_descriptor&& other) {
        std::swap(fd_, other.fd_);
    }

    file_descriptor& operator=(file_descriptor const&) = delete;

    file_descriptor& operator=(file_descriptor&& other) {
        file_descriptor(std::move(other)).swap(*this);
        return *this;
    }

    ~file_descriptor() {
        try {
            release();
        } catch (std::exception const& e) {
            fmt::print(stderr, FMT_COMPILE("{}\n"), e.what());
        }
    }

    static file_descriptor open(char const* filename, int flags) {
        file_descriptor fd(::open(filename, flags));
        if (!fd) {
            throw_errno("open");
        }
        return fd;
    }

    void reset(int fd) {
        file_descriptor(fd).swap(*this);
    }

    void swap(file_descriptor& other) {
        std::swap(fd_, other.fd_);
    }

    void release() {
        if (auto const fd = std::exchange(fd_, -1); fd >= 0) {
            if (close(fd)) {
                throw_errno("close");
            }
        }
    }

    int get() const {
        return fd_;
    }

    explicit operator bool() const {
        return fd_ >= 0;
    }

private:
    int fd_ = -1;
};

struct file {
private:
    explicit file(std::string const& filename, bool is_temp, bool is_readonly)
        : fd_(), filename_(filename), is_temp_(is_temp), readonly_(is_readonly) {}

    explicit file(file_descriptor&& fd, std::string const& filename, bool is_temp,
                  bool is_readonly)
        : fd_(std::move(fd)), filename_(filename), is_temp_(is_temp), readonly_(is_readonly) {}

    static file_descriptor open_ro(std::string const& filename) {
        file_descriptor fd(::open(filename.c_str(), O_RDONLY | O_CLOEXEC));
        if (!fd) {
            throw_errno_fmt("open: {}", filename);
        }
        return fd;
    }

    static file_descriptor open_rw(std::string const& filename) {
        file_descriptor fd(::open(filename.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644));
        if (!fd) {
            throw_errno_fmt("open: {}", filename);
        }
        return fd;
    }

public:
    file(file const&) = delete;

    file(file&& other) = default;

    file& operator=(file const&) = delete;

    file& operator=(file&& other) = default;

    ~file() {
        try {
            release();
        } catch (std::exception const& e) {
            fmt::print(stderr, FMT_COMPILE("{}\n"), e.what());
        }
    }

    static file read_only(std::string const& filename) {
        return file(open_ro(filename), filename, false, true);
    }

    static file open(std::string const& filename) {
        return file(open_rw(filename), filename, false, false);
    }

    static file mktemp(std::string_view ext) {
        return mktemp(ext.data(), ext.size());
    }

    static file mktemp(char const* ext = nullptr) {
        return mktemp(ext, ::strlen(ext));
    }

    static file mktemp(char const* ext, size_t extlen) {
        std::string filename = "/tmp/tmpXXXXXX";
        file_descriptor fd;

        if (extlen > 0) {
            filename.append(ext);
            fd.reset(::mkostemps(const_cast<char*>(filename.data()), extlen, O_CLOEXEC));
        } else {
            fd.reset(::mkostemps(const_cast<char*>(filename.data()), 0, O_CLOEXEC));
        }
        if (!fd) {
            throw_errno("mkstemp");
        }

        return file(std::move(fd), filename, true, false);
    }

    std::string read_contents(off_t off, size_t len) const {
        std::string res;
        res.resize(len);

        auto const nr = pread(fd_.get(), res.data(), res.size(), off);

        if (nr < 0 || nr != static_cast<ssize_t>(len)) {
            throw_errno_fmt("pread: {}", filename_);
        }

        return res;
    }

    std::string read_contents() const {
        std::vector<char> buffer(32 * 1024);
        std::string res;
        off_t off = 0;

        for (;;) {
            auto const nr = pread(fd_.get(), buffer.data(), buffer.size(), off);

            if (nr < 0) {
                throw_errno_fmt("pread: {}", filename_);
            }
            if (nr == 0) {
                break;
            }

            res.append(buffer.data(), nr);
            off += nr;
        }

        return res;
    }

    std::array<char, 8> read_header() const {
        std::array<char, 8> buffer;
        buffer.fill(0);

        auto const nr = pread(fd_.get(), buffer.data(), buffer.size(), 0);
        if (nr < 0 || nr != static_cast<ssize_t>(buffer.size())) {
            throw_errno_fmt("pread: {}", filename_);
        }

        return buffer;
    }

    off_t write_content(char const* ptr, size_t len, off_t off = 0) const {
        auto const nw = pwrite(fd_.get(), ptr, len, off);
        if (nw != static_cast<ssize_t>(len)) {
            throw_errno_fmt("pwrite: {}", filename_);
        }

        off += len;

        if (ftruncate(fd_.get(), off)) {
            throw_errno_fmt("ftruncate: {}", filename_);
        }

        return off;
    }

    off_t write_content(std::string const& data, off_t off = 0) const {
        return write_content(data.data(), data.size(), off);
    }

    off_t copy_from(file const& src, off_t write_off = 0, off_t read_off = 0) const {
        for (;;) {
            std::vector<char> buffer(32 * 1024);
            auto const nr = pread(src.fd_.get(), buffer.data(), buffer.size(), read_off);

            if (nr == 0) {
                break;
            }
            if (nr < 0) {
                throw_errno_fmt("pread: {}", filename_);
            }

            auto const nw = pwrite(fd_.get(), buffer.data(), nr, write_off);
            if (nw != nr) {
                throw_errno_fmt("write: {}", filename_);
            }

            read_off += nr;
            write_off += nr;
        }

        if (ftruncate(fd_.get(), write_off)) {
            throw_errno_fmt("ftruncate: {}", filename_);
        }

        return write_off;
    }

    int fd() const {
        return fd_.get();
    }

    std::string const& filename() const {
        return filename_;
    }

    std::string ext() const {
        return std::filesystem::path(filename_).extension();
    }

    void release() {
        auto const fd = std::exchange(fd_, {});
        auto const filename = std::exchange(filename_, {});
        if (fd && is_temp_) {
            if (unlink(filename.c_str())) {
                throw_errno_fmt("unlink: {}", filename);
            }
        }
    }

    void swap(file& other) {
        std::swap(fd_, other.fd_);
        std::swap(filename_, other.filename_);
        std::swap(is_temp_, other.is_temp_);
        std::swap(readonly_, other.readonly_);
    }

    struct stat stat() const {
        struct stat st;
        if (fstat(fd_.get(), &st)) {
            throw_errno_fmt("fstat: {}", filename_);
        }
        return st;
    }

    void reopen() {
        if (readonly_) {
            fd_ = open_ro(filename_);
        } else {
            fd_ = open_rw(filename_);
        }
    }

private:
    file_descriptor fd_;
    std::string filename_;
    bool is_temp_ = false;
    bool readonly_;
};

bool has_next_arg(int i, int argc) {
    return i + 1 < argc;
}

char* require_next_arg(int& i, int argc, char** argv) {
    if (!has_next_arg(i, argc)) {
        fmt::print(stderr, FMT_COMPILE("Error: option requires a value: {}\n"), argv[i]);
        throw exit_failure();
    }
    i++;
    return argv[i];
}

struct fcloser {
    void operator()(FILE* fp) const {
        fclose(fp);
    }
};

struct workflow {
    explicit workflow(config const& cfg)
        : cfg_(cfg), devnull_(file_descriptor::open("/dev/null", O_RDWR)) {}

    void run() {
        std::vector<file> objects;
        for (auto const& input : cfg_.inputs) {
            process_input(objects, input);
        }

        if (cfg_.stop_at == "codegen") {
            return;
        }

        link_objects(objects);
    }

private:
    using file_map = std::unordered_multimap<u::target, file>;

    struct run_command_opt {
        int stdin = -1;
        int stdout = -1;
        int stderr = -1;
        mode_t umask = 0077;
        bool check = true;
    };

    bool test_command(std::vector<std::string> const& args) const {
        run_command_opt opt;
        opt.stdin = devnull_.get();
        opt.stdout = devnull_.get();
        opt.stderr = devnull_.get();
        return run_command(args, opt);
    }

    bool run_command(std::vector<std::string> const& args) const {
        return run_command(args, run_command_opt());
    }

    bool run_command(std::vector<std::string> const& args, run_command_opt const& opt) const {
        std::vector<char*> ptrs;
        for (auto const& arg : args) {
            ptrs.push_back(const_cast<char*>(arg.data()));
        }

        if (ptrs.empty() || ptrs.back() != nullptr) {
            ptrs.push_back(nullptr);
        }

        if (cfg_.verbose) {
            std::string cmdline;

            for (auto const* arg : ptrs) {
                if (arg) {
                    cmdline += fmt::format(FMT_COMPILE("{} "), arg);
                }
            }

            begin_step(cmdline);
        } else {
            ++step_;
        }

        auto const t_start = std::chrono::high_resolution_clock::now();

        auto const pid = fork();
        if (pid == 0) {
            if (opt.stdin >= 0 && opt.stdin != STDIN_FILENO) {
                if (dup2(opt.stdin, STDIN_FILENO) == -1) {
                    throw_errno("dup2");
                }
            }

            if (opt.stdout >= 0 && opt.stdout != STDOUT_FILENO) {
                if (dup2(opt.stdout, STDOUT_FILENO) == -1) {
                    throw_errno("dup2");
                }
            }

            if (opt.stderr >= 0 && opt.stderr != STDERR_FILENO) {
                if (dup2(opt.stderr, STDERR_FILENO) == -1) {
                    throw_errno("dup2");
                }
            }

            if (opt.umask != static_cast<mode_t>(-1)) {
                umask(opt.umask);
            }

            if (execvp(ptrs.at(0), ptrs.data()) == -1) {
                throw_errno("exec");
            }

            std::terminate();
        } else if (pid != -1) {
            int status;
            if (wait(&status) == -1) {
                throw_errno("wait");
            }

            if (!WIFEXITED(status)) {
                throw exit_failure();
            }

            if (opt.check) {
                if (WEXITSTATUS(status) != 0) {
                    throw exit_failure();
                }
            }

            if (cfg_.verbose) {
                auto const t_end = std::chrono::high_resolution_clock::now();
                fmt::print(FMT_COMPILE("        {:.2f} sec\n"), delta_sec(t_start, t_end));
            }

            return WEXITSTATUS(status) == 0;
        } else {
            throw_errno("fork");
        }
    }

    void process_input(std::vector<file>& objs, std::string const& input_file) {
        if (has_ext(input_file, ".cpp") || has_ext(input_file, ".cc")) {
            process_sycl_input(objs, input_file);
            return;
        }
        if (has_ext(input_file, ".o") || has_ext(input_file, ".a")) {
            objs.push_back(file::read_only(input_file));
            return;
        }

        fmt::print(stderr, FMT_COMPILE("Not supported input file: {}\n"), input_file.c_str());
        throw exit_failure();
    }

    void process_sycl_input(std::vector<file>& objs, std::string const& input_file) {
        auto const dev_cpp = run_cpp(input_file, false);

        auto const kernel_xmls = run_kext(dev_cpp.filename());
        auto const lower_xmls = run_lower(kernel_xmls);

        auto const kernel_srcs = run_cback(lower_xmls);

        if (cfg_.stop_at == "codegen") {
            return;
        }

        auto kernel_objs = compile_kernel(kernel_srcs);

        auto host_cpp = run_cpp(input_file, true);
        host_cpp = transform_host(host_cpp.filename(), kernel_objs);
        auto host_obj = compile_host(host_cpp.filename(), NO_PIC);

        objs.push_back(std::move(host_obj));
        for (auto& pair : kernel_objs) {
            objs.push_back(std::move(pair.second));
        }
    }

    void save_temps_impl(file const& f, std::string const& outfile, filetype type) const {
        auto const do_save = cfg_.save_temps ||
                             (cfg_.save_kernels && type == filetype::kernel) ||
                             (cfg_.save_xmls && type == filetype::xml);

        if (!do_save) {
            return;
        }

        if (cfg_.verbose) {
            fmt::print("        copy from {} to {}\n", f.filename(), outfile);
        }

        file::open(outfile).copy_from(f);
    }

    void save_temps(file const& f, filetype type) const {
        save_temps(f, get_ext(f.filename()), type);
    }

    void save_temps(file const& f, std::string_view ext, filetype type) const {
        save_temps_impl(
            f, fmt::format(FMT_COMPILE("{}.{}{}"), trim_ext(cfg_.output), step_, ext), type);
    }

    void save_temps(file const& f, u::target t, filetype type) const {
        save_temps(f, get_ext(f.filename()), t, type);
    }

    void save_temps(file const& f, std::string_view ext, u::target t, filetype type) const {
        save_temps_impl(f,
                        fmt::format(FMT_COMPILE("{}.{}.{}{}"), trim_ext(cfg_.output), step_,
                                    u::show(t), ext),
                        type);
    }

    file run_clang_format(file input, char const* ext) const {
        if (cfg_.clang_format.empty()) {
            return input;
        }

        auto out = file::mktemp(ext);
        run_command_opt opt;
        opt.stdout = out.fd();
        run_command({cfg_.clang_format, "--style=Chromium", input.filename()}, opt);

        return out;
    }

    file_map run_kext(std::string const& input_file) {
        file_map outs;

        auto out = file::mktemp(".xml");

        run_command({cfg_.tool_path("kext", "charm-kext"), "--output", out.filename(),
                     input_file, "--", "-Xclang", "-fsycl-is-device", "-std=" + cfg_.cxx_std});
        save_temps(out, filetype::xml);

        auto const st = out.stat();
        if (st.st_size > 0) {
#ifdef USE_IRIS
            bool symbols_collected = false;
#endif

            for (auto const t : cfg_.targets) {
                auto out2 = file::mktemp(".xml");
                out2.copy_from(out);

#ifdef USE_IRIS
                if (!symbols_collected && is_cpu(t)) {
                    collect_symbols(out2);
                    symbols_collected = true;
                }
#endif

                outs.emplace(t, std::move(out2));
            }
        }

        return outs;
    }

#ifdef USE_IRIS
    void collect_symbols(file const& input) {
        auto out = file::mktemp(".txt");

        run_command({cfg_.tool_path("chsy-get", "chsy-get"), "--kernels", "--output",
                     out.filename(), input.filename()});
        save_temps(out, filetype::other);

        auto const list = out.read_contents();

        if (list.empty()) {
            return;
        }

        for (size_t pos = 0; pos < list.size();) {
            auto const nl = list.find("\n", pos);

            auto const end = nl == std::string::npos ? list.size() : nl;
            auto const name = list.substr(pos, end - pos);

            if (!name.empty()) {
                cpu_symbols.push_back(name);
            }

            pos = end + 1;
        }
    }
#endif

    file run_cpp(std::string const& input_file, bool for_host) const {
        auto out = file::mktemp(".cpp");

        std::vector<std::string> cmd({cfg_.cxx, "-E", "-Xclang",
                                      for_host ? "-fsycl-is-host" : "-fsycl-is-device",
                                      "-std=" + cfg_.cxx_std, "-w", "-o", "-", input_file});
#ifdef CSCC_USE_LIBCXX
        cmd.push_back(fmt::format("-stdlib=libc++"));
#endif

        if (auto const inc = cfg_.include_path_src(); !inc.empty()) {
            cmd.push_back("-I" + inc);
        }
        if (auto const inc = cfg_.include_path_bin(); !inc.empty()) {
            cmd.push_back("-I" + inc);
        }
        for (auto const& dir : cfg_.include_dirs) {
            cmd.push_back("-I" + dir);
        }
        for (auto const& [k, v] : cfg_.defines) {
            if (v.empty()) {
                cmd.push_back(std::string("-D") + k);
            } else {
                cmd.push_back(std::string("-D") + k + "=" + v);
            }
        }
        if (for_host) {
            if (cfg_.md) {
                cmd.push_back("-MD");
            }
            if (cfg_.mmd) {
                cmd.push_back("-MMD");
            }
            if (cfg_.mm) {
                cmd.push_back("-MM");
            }
            if (cfg_.m) {
                cmd.push_back("-M");
            }
            if (!cfg_.mf.empty()) {
                cmd.push_back("-MF");
                cmd.push_back(cfg_.mf);
            }
            if (!cfg_.mt.empty()) {
                cmd.push_back("-MT");
                cmd.push_back(cfg_.mt);
            }
        }
        if (cfg_.debug) {
            cmd.push_back("-g");
        }

        run_command_opt opt;
        opt.stdout = out.fd();

        run_command(cmd, opt);
        save_temps(out, filetype::other);

        return out;
    }

    file transform_host(std::string const& input_file, file_map const& kernels) const {
        auto out = file::mktemp(".cpp");

        if (kernels.empty()) {
            begin_step(fmt::format("cp {} {} (internal)", input_file, out.filename()));
            out.copy_from(file::read_only(input_file));
        } else {
            run_command({cfg_.tool_path("chsy-host-rewrite", "chsy-host-rewrite"), "--output",
                         out.filename(), input_file, "--", "-Xclang", "-fsycl-is-host",
                         "-std=" + cfg_.cxx_std});
        }

        save_temps(out, filetype::other);

        return out;
    }

    [[maybe_unused]] static constexpr bool PIC = true, NO_PIC = false;

    file compile_host(std::string const& input_file, bool pic) const {
        auto out = file::mktemp(".o");

        std::vector<std::string> cmd({cfg_.cxx, "-Xclang", "-fsycl-is-host",
                                      "-std=" + cfg_.cxx_std, "-o", out.filename().c_str(),
                                      "-c", input_file});

        if (pic) {
            cmd.push_back("-fPIC");
        }

#ifdef CSCC_USE_LIBCXX
        cmd.push_back(fmt::format("-stdlib=libc++"));
#endif

        cmd.push_back(std::string("-O") + cfg_.opt_level);
        if (cfg_.debug) {
            cmd.push_back("-g");
        }
        if (!cfg_.fsanitize.empty()) {
            cmd.push_back("-fsanitize=" + cfg_.fsanitize);
        }

        run_command(cmd);
        out.reopen();

        return out;
    }

    file_map run_lower(file_map const& input_files) const {
        file_map outs;

        for (auto const& [target, input] : input_files) {
            auto out = file::mktemp(".xml");

            run_command({cfg_.tool_path("chsy-lower", "chsy-lower"), "--output", out.filename(),
                         "--target", show(target), input.filename()});
            save_temps(out, target, filetype::xml);

            outs.emplace(target, std::move(out));
        }

        return outs;
    }

    file_map run_cback(file_map const& input_files) const {
        file_map outs;

        for (auto const& [target, input] : input_files) {
            auto out = file::mktemp(kernel_ext(target).c_str());

            run_command({cfg_.tool_path("chsy-c-back", "chsy-c-back"), "--output",
                         out.filename(), "--target", show(target), input.filename()});

            out = run_clang_format(std::move(out), kernel_ext(target).c_str());

            save_temps(out, target, filetype::kernel);

            outs.emplace(target, std::move(out));
        }

        return outs;
    }

    file_map compile_kernel(file_map const& input_files) const {
        file_map outs;

        for (auto const& [target, input] : input_files) {
            switch (target) {
                case u::target::NONE:
                    break;

                case u::target::CPU_C:
                case u::target::CPU_OPENMP:
                    outs.emplace(target, compile_kernel_cc(input, target));
                    continue;

                case u::target::NVIDIA_CUDA: {
                    auto fatbin = compile_kernel_cuda(input, cudafmt::FATBIN);
                    auto fatbin_obj = embed_file(fatbin);
                    fatbin_obj.first = mark_object(fatbin_obj.first, target);
                    auto fatbin_c = make_binary_loader(fatbin_obj, "_FATBIN_");
                    outs.emplace(target, std::move(fatbin_obj.first));
                    outs.emplace(target, std::move(fatbin_c));

#ifdef USE_IRIS
                    auto ptx = compile_kernel_cuda(input, cudafmt::PTX);
                    auto ptx_obj = embed_file(ptx);
                    ptx_obj.first = mark_object(ptx_obj.first, target);
                    auto ptx_c = make_binary_loader(ptx_obj, "_PTX_");
                    outs.emplace(target, std::move(ptx_obj.first));
                    outs.emplace(target, std::move(ptx_c));
#endif
                    continue;
                }

                case u::target::AMD_HIP: {
                    auto hsaco = compile_kernel_hipcc(input);
                    auto hsaco_obj = embed_file(hsaco);
                    hsaco_obj.first = mark_object(hsaco_obj.first, target);
                    auto hsaco_c = make_binary_loader(hsaco_obj, "_HSACO_");
                    outs.emplace(target, std::move(hsaco_obj.first));
                    outs.emplace(target, std::move(hsaco_c));
                    continue;
                }
            }
            std::terminate();
        }

        return outs;
    }

    void link_objects(std::vector<file>& objs) const {
#ifdef USE_IRIS
        if (cfg_.do_link) {
            prepare_iris(objs);
        }
#endif

        if (cfg_.do_link) {
            link_binary(objs);
        } else {
            link_archive(objs);
        }
    }

#ifdef USE_IRIS
    std::vector<std::string> cpu_symbols;

    bool targets_openmp() const {
        for (auto const& t : cfg_.targets) {
            if (is_openmp(t)) {
                return true;
            }
        }
        return false;
    }

    void prepare_iris(std::vector<file>& out_objs) const {
        std::string sym_c;
        for (auto const& sym : cpu_symbols) {
            sym_c += fmt::format("extern \"C\" void {}() {{}}\n", sym);
        }

        auto sym = file::mktemp(".cpp");
        begin_step(fmt::format("Generate symbol table {}", sym.filename()));
        sym.write_content(sym_c);
        save_temps(sym, filetype::other);

        std::vector<file> objs;
        objs.push_back(file::read_only(cfg_.iris_omp_loader()));
        objs.push_back(compile_host(sym.filename(), PIC));

        auto loader = link_so(objs, targets_openmp());
        save_temps(loader, filetype::other);
        auto loader_obj = embed_file(loader);
        auto loader_c = make_binary_loader(loader_obj, "_IRIS_OMP_LOADER_");
        out_objs.push_back(std::move(loader_obj.first));
        out_objs.push_back(std::move(loader_c));
    }
#endif

    file link_so(std::vector<file> const& inputs, bool openmp) const {
        auto out = file::mktemp(".so");

        std::vector<std::string> cmd({cfg_.linker, "-shared", "-o", out.filename()});

#ifdef CSCC_APPLE
        cmd.push_back("-undefined");
        cmd.push_back("dynamic_lookup");
#endif

#ifdef CSCC_USE_LINKER
        cmd.push_back(fmt::format("-fuse-ld={}", CSCC_USE_LINKER));
#endif

#ifdef CSCC_USE_LIBCXX
        cmd.push_back(fmt::format("-stdlib=libc++"));
#endif

        for (auto const& file : inputs) {
            cmd.push_back(file.filename());
        }

        if (openmp) {
#ifdef OpenMP_FOUND
            cmd.push_back(OpenMP_C_FLAGS);
#endif
        }

#ifdef CSCC_USE_LIBCXX_STATIC
        cmd.push_back(fmt::format("-static-libstdc++"));
#    ifdef CSCC_LIBCXXABI_PATH
        cmd.push_back(CSCC_LIBCXXABI_PATH);
#    else
        cmd.push_back("-lc++abi");
#    endif
#endif

        run_command(cmd);
        out.reopen();

        return out;
    }

    void link_binary(std::vector<file>& inputs) const {
        std::vector<std::string> cmd({cfg_.linker, "-o", cfg_.output});
        std::vector<file> sycl_libs, objs;
        bool has_sycl_objects = false;
        bool uses_openmp = false;

        for (auto& input : inputs) {
            auto const targets = targets_from_object(input);
            auto const is_sycl_obj = !targets.empty();

            if (is_sycl_obj) {
                has_sycl_objects = true;
                input = drop_marker(input);
            }

            if (has_ext(input.filename(), ".a") && is_sycl_obj) {
                sycl_libs.push_back(std::move(input));
            } else {
                objs.push_back(std::move(input));
            }

            for (auto const& target : targets) {
                if (is_openmp(target)) {
                    uses_openmp = true;
                }
            }
        }

        if (!cfg_.fsanitize.empty()) {
            cmd.push_back("-fsanitize=" + cfg_.fsanitize);
        }

#ifndef CSCC_APPLE
        cmd.push_back("-Wl,--start-group");
#endif

        if (has_sycl_objects) {
#if defined(USE_IRIS) && !defined(CSCC_APPLE)
            cmd.push_back("-rdynamic");
#endif

#ifndef CSCC_APPLE
            cmd.push_back("-Wl,--whole-archive");
#endif

            for (auto const& f : sycl_libs) {
                cmd.push_back(f.filename());
            }

#ifndef CSCC_APPLE
            cmd.push_back("-Wl,--no-whole-archive");
#endif
        }

        for (auto const& obj : objs) {
            cmd.push_back(obj.filename());
        }

        auto const runtime = cfg_.sycl_runtime_library();
#ifdef CHARM_SYCL_IS_SHARED_LIBRARY
        cmd.push_back(fmt::format("-Wl,-rpath={}", runtime.parent_path()));
#endif
        cmd.push_back(runtime);

#ifndef CSCC_APPLE
        cmd.push_back("-Wl,--end-group");
#endif

#ifdef USE_IRIS
        cmd.push_back(IRIS_LIBRARY);
#endif

#ifdef CUDAToolkit
        cmd.push_back(cfg_.cuda_library_path);
        cmd.push_back("-ldl");
        cmd.push_back("-lrt");
#endif

#ifdef hip_FOUND
        cmd.push_back(fmt::format("-Wl,-rpath={}", HIP_LIB_INSTALL_DIR));
        cmd.push_back("-L" HIP_LIB_INSTALL_DIR);
        cmd.push_back("-lamdhip64");
#endif

        cmd.push_back("-pthread");
        cmd.push_back("-lm");

        for (auto const& dir : cfg_.library_dirs) {
            cmd.push_back("-L" + dir);
        }
        for (auto const& lib : cfg_.libraries) {
            cmd.push_back("-l" + lib);
        }

        if (uses_openmp) {
#ifdef OpenMP_FOUND
            cmd.push_back(OpenMP_C_FLAGS);
#endif
        }

#ifdef CSCC_USE_LINKER
        cmd.push_back(fmt::format("-fuse-ld={}", CSCC_USE_LINKER));
#endif
#ifdef CSCC_USE_LIBCXX
        cmd.push_back(fmt::format("-stdlib=libc++"));
#endif
#ifdef CSCC_USE_LIBCXX_STATIC
        cmd.push_back(fmt::format("-static-libstdc++"));
#    ifdef CSCC_LIBCXXABI_PATH
        cmd.push_back(CSCC_LIBCXXABI_PATH);
#    else
        cmd.push_back("-lc++abi");
#    endif
#endif

        run_command_opt opt;
        opt.umask = -1;
        run_command(cmd);
    }

    void link_archive(std::vector<file> const& inputs) const {
        auto lib = file::mktemp(".a");

        std::vector<std::string> cmd({cfg_.ar, "cqs", lib.filename()});

        for (auto const& input : inputs) {
            cmd.push_back(input.filename());
        }

        run_command_opt opt;
        opt.umask = -1;
        run_command(cmd, opt);

        lib = mark_object(lib, cfg_.targets);

        file::open(cfg_.output).copy_from(lib);
    }

    static bool is_marked_object(file const& file) {
        auto const header = file.read_header();
        return header[0] == 'C' && header[1] == 'H' && header[2] == 'S' && header[3] == 'Y';
    }

    file mark_object(file const& input_file, u::target target) const {
        return mark_object(input_file, std::vector({target}));
    }

    file mark_object(file const& input_file, std::vector<u::target> targets) const {
        file out = file::mktemp(input_file.ext());

        std::string t_str;
        for (auto const& t : targets) {
            if (!t_str.empty()) {
                t_str += ',';
            }
            t_str += show(t);
        }

        begin_step(fmt::format("Generate marked object {} from {}", out.filename(),
                               input_file.filename()));

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
        off = out.write_content(header.data(), header.size(), off);
        off = out.write_content("\0", 1, off);
        off = out.write_content(t_str, off);
        off = out.write_content("\0", 1, off);
        off = out.copy_from(input_file, off);

        return out;
    }

    std::vector<u::target> targets_from_object(file const& input_file) const {
        if (!is_marked_object(input_file)) {
            return {};
        }

        auto const header = input_file.read_header();
        auto const len = static_cast<size_t>(header[4]) | static_cast<size_t>(header[5]) << 8 |
                         static_cast<size_t>(header[6]) << 16 |
                         static_cast<size_t>(header[7]) << 24;

        return u::from_list_string(input_file.read_contents(9, len));
    }

    file drop_marker(file const& input) const {
        auto out = file::mktemp(input.ext());
        auto const header = input.read_header();
        auto const len = static_cast<size_t>(header[4]) | static_cast<size_t>(header[5]) << 8 |
                         static_cast<size_t>(header[6]) << 16 |
                         static_cast<size_t>(header[7]) << 24;

        begin_step(
            fmt::format("Extract marked object {} from {}", out.filename(), input.filename()));

        out.copy_from(input, 0, 8 + 1 + len + 1);

        return out;
    }

    file compile_kernel_cc(file const& input, u::target target) const {
        auto out = file::mktemp(".o");

        std::vector<std::string> cmd(
            {cfg_.cc_for_kernel(), "-c", "-o", out.filename(), input.filename()});

#if OpenMP_FOUND
        if (is_openmp(target)) {
            cmd.push_back(OpenMP_C_FLAGS);
        }
#endif

        cmd.push_back(std::string("-O") + cfg_.opt_level);
        if (cfg_.debug) {
            cmd.push_back("-g");
        }
        if (!cfg_.fsanitize.empty()) {
            cmd.push_back("-fsanitize=" + cfg_.fsanitize);
        }

        for (auto const& dir : cfg_.include_dirs) {
            cmd.push_back("-I" + dir);
        }
        for (auto const& [k, v] : cfg_.defines) {
            if (v.empty()) {
                cmd.push_back(std::string("-D") + k);
            } else {
                cmd.push_back(std::string("-D") + k + "=" + v);
            }
        }

        cmd.insert(cmd.end(), cfg_.remainder.begin(), cfg_.remainder.end());

        run_command(cmd);
        out.reopen();

        return mark_object(out, target);
    }

    enum class cudafmt { OBJECT, FATBIN, PTX };

    static char const* ext_for(cudafmt fmt) {
        switch (fmt) {
            case cudafmt::OBJECT:
                return ".o";
            case cudafmt::FATBIN:
                return ".fatbin";
            case cudafmt::PTX:
                return ".ptx";
        }
        std::terminate();
    }

    static std::string_view first_line(std::string_view str) {
        auto const newline = str.find("\n");
        if (newline == std::string_view::npos) {
            return str;
        }
        return str.substr(0, newline);
    }

    std::pair<file, std::string> embed_file([[maybe_unused]] file const& input) const {
#ifndef OBJCOPY_OUTPUT_FORMAT
        auto const errmsg = fmt::format(
            "embedding a file is not supported for CMAKE_SYSTEM_NAME={} "
            "and CMAKE_SYSTEM_PROCESSOR={}",
            CMAKE_SYSTEM_NAME, CMAKE_SYSTEM_PROCESSOR);
        throw std::runtime_error(errmsg);
#else
        auto out = file::mktemp(".o");

        run_command({cfg_.objcopy, "-I", "binary", "-O", OBJCOPY_OUTPUT_FORMAT,
                     input.filename(), out.filename()});
        out.reopen();
        save_temps(out, filetype::other);

        auto syms = file::mktemp(".txt");
        run_command_opt opts;
        opts.stdout = syms.fd();
        run_command({cfg_.nm, out.filename()}, opts);
        save_temps(syms, filetype::other);

        // 000000000000006d D _binary_XXXXX_end
        auto const nm_output = syms.read_contents();
        auto const sym_data = first_line(nm_output);
        auto const name_start = sym_data.rfind(" ") + 1;
        auto const name_end = sym_data.rfind("_") + 1;
        auto const name_pre = sym_data.substr(name_start, name_end - name_start);

        return std::make_pair(std::move(out), std::string(name_pre));
#endif
    }

    file make_binary_loader(std::pair<file, std::string> const& pair,
                            std::string_view kind) const {
        std::vector<char> buffer;
        auto it = std::back_inserter(buffer);

        fmt::format_to(it, "#include <cstdlib>\n");
        fmt::format_to(it, "#include <cstdint>\n");
        fmt::format_to(it, "struct bin_info {{ void* ptr; ssize_t len; }};\n");
        fmt::format_to(it, "static struct bin_info bin;\n");
        fmt::format_to(it,
                       "extern \"C\" void __s_add_kernel_registry(char const*, uint32_t, char "
                       "const*, uint32_t, void*);\n");
        fmt::format_to(it, "extern char {}start;\n", pair.second);
        fmt::format_to(it, "extern char {}end;\n", pair.second);

        auto const* name = "";
        auto const name_hash = u::fnv1a(name);
        auto const kind_hash = u::fnv1a(kind.data(), kind.size());

        fmt::format_to(it, "__attribute__((constructor)) static void do_register() {{\n");
        fmt::format_to(it, "bin.ptr = &{}start; bin.len = &{}end - &{}start;\n", pair.second,
                       pair.second, pair.second);
        fmt::format_to(it,
                       "__s_add_kernel_registry(\"{}\", UINT32_C(0x{:x}), \"{}\", "
                       "UINT32_C(0x{:x}), &bin);\n",
                       name, name_hash, kind, kind_hash);
        fmt::format_to(it, "\n}}\n");

        auto out = file::mktemp(".cpp");

        begin_step(fmt::format("Generate binary loader: {} from {}", out.filename(),
                               pair.first.filename()));
        out.write_content(buffer.data(), buffer.size());
        save_temps(out, filetype::other);

        return compile_host(out.filename(), NO_PIC);
    }

    file compile_kernel_hipcc([[maybe_unused]] file const& input) const {
#ifndef hip_FOUND
        fmt::print(stderr, FMT_COMPILE("HIP compilation is not enabled\n"));
        throw exit_failure();
#else
        auto out = file::mktemp(".hsaco");

        std::vector<std::string> cmd(
            {cfg_.hipcc, "--genco", "-o", out.filename(), input.filename()});

        cmd.push_back("-std=c++17");
        cmd.push_back(std::string("-O") + cfg_.opt_level);
        if (cfg_.debug) {
            cmd.push_back("-g");
        }

        run_command(cmd);
        save_temps(out, filetype::other);

        return out;
#endif
    }

    file compile_kernel_cuda([[maybe_unused]] file const& input,
                             [[maybe_unused]] cudafmt fmt) const {
#ifndef CUDAToolkit
        fmt::print(stderr, FMT_COMPILE("CUDA compilation is not enabled\n"));
        throw exit_failure();
#else
        auto out = file::mktemp(ext_for(fmt));

        std::vector<std::string> cmd({cfg_.nvcc, "-c", "-o", out.filename(), input.filename()});

        switch (fmt) {
            case cudafmt::FATBIN:
                cmd.push_back("--fatbin");
                break;
            case cudafmt::PTX:
                cmd.push_back("--ptx");
                break;
            default:
                break;
        }

        if (!cfg_.ccbin.empty()) {
            cmd.push_back("-ccbin");
            cmd.push_back(cfg_.ccbin);
        }

        if (!cfg_.cuda_arch.empty()) {
            cmd.push_back("-arch=" + cfg_.cuda_arch);
        }

#    ifdef CSCC_USE_LIBCXX
        // cmd.push_back("-Xcompiler");
        // cmd.push_back(fmt::format("-stdlib=libc++"));
#    endif

        cmd.push_back("-std=" + cfg_.cxx_std);
        cmd.push_back(std::string("-O") + cfg_.opt_level);
        if (cfg_.debug) {
            cmd.push_back("-g");
        }
        for (auto const& dir : cfg_.include_dirs) {
            cmd.push_back("-I" + dir);
        }
        for (auto const& [k, v] : cfg_.defines) {
            if (v.empty()) {
                cmd.push_back(std::string("-D") + k);
            } else {
                cmd.push_back(std::string("-D") + k + "=" + v);
            }
        }
        if (!cfg_.fsanitize.empty()) {
            cmd.push_back("-Xcompiler");
            cmd.push_back("-fsanitize=" + cfg_.fsanitize);
        }

        cmd.insert(cmd.end(), cfg_.remainder.begin(), cfg_.remainder.end());

        run_command(cmd);
        save_temps(out, filetype::other);

        return out;
#endif
    }

    std::string kernel_ext(u::target t) const {
        switch (t) {
            case u::target::NONE:
                break;
            case u::target::CPU_C:
            case u::target::CPU_OPENMP:
                return ".c";
            case u::target::NVIDIA_CUDA:
                return ".cu";
            case u::target::AMD_HIP:
                return ".cpp";
        }
        std::terminate();
    }

    void begin_step(std::string const& msg) const {
        step_++;
        if (cfg_.verbose) {
            fmt::print(FMT_COMPILE("  {:3d}: {}\n"), step_, msg);
        }
    }

    config const& cfg_;
    file_descriptor devnull_;
    int mutable step_ = 0;
};

}  // namespace

#define SHORT_OPT(name)              \
    if (!matched && arg == "-" name) \
        if ((matched = true))

#define LONG_OPT(name)                                                    \
    if (!matched && (has_prefix(arg, "--" name "=") || arg == "--" name)) \
        if ((matched = true))

#define SHORT_OPT_VAL(name)                                                           \
    if (!matched && arg == "-" name)                                                  \
        if ([[maybe_unused]] auto const optval = sv(require_next_arg(i, argc, argv)); \
            (matched = true))

#define PREFIX_OPT_VAL(name)                                                          \
    if (!matched && has_prefix(arg, "-" name))                                        \
        if ([[maybe_unused]] auto const optval =                                      \
                (arg == "-" name)               ? sv(require_next_arg(i, argc, argv)) \
                : has_prefix(arg, "-" name "=") ? arg.substr(strlen("-" name "="))    \
                                                : arg.substr(strlen("-" name));       \
            (matched = true))

#define LONG_OPT_VAL(name)                                                              \
    if (!matched && (has_prefix(arg, "-" name "=") || arg == "-" name ||                \
                     has_prefix(arg, "--" name "=") || arg == "--" name))               \
        if ([[maybe_unused]] auto const optval =                                        \
                has_prefix(arg, "-" name "=")    ? arg.substr(strlen("-" name "="))     \
                : has_prefix(arg, "--" name "=") ? arg.substr(strlen("--" name "="))    \
                                                 : sv(require_next_arg(i, argc, argv)); \
            (matched = true))

int main(int argc, char** argv) {
    try {
        auto const t_start = std::chrono::high_resolution_clock::now();
        config cfg;

        for (int i = 1; i < argc; i++) {
            auto const arg = sv(argv[i]);
            bool matched = false;

            if (arg == "--") {
                cfg.remainder.insert(cfg.remainder.end(), argv + i + 1, argv + argc);
                break;
            }

            SHORT_OPT("c") {
                cfg.do_link = false;
            }
            LONG_OPT_VAL("cc") {
                cfg.cc = optval;
            }
            LONG_OPT_VAL("kcc") {
                cfg.k_cc = optval;
            }
            LONG_OPT_VAL("cxx") {
                cfg.cxx = optval;
            }
            LONG_OPT_VAL("ar") {
                cfg.ar = optval;
            }
            LONG_OPT_VAL("clang-format") {
                cfg.clang_format = optval;
            }
            SHORT_OPT_VAL("o") {
                cfg.output = optval;
            }
            SHORT_OPT("O0") {
                cfg.opt_level = '0';
            }
            SHORT_OPT("O1") {
                cfg.opt_level = '1';
            }
            SHORT_OPT("O2") {
                cfg.opt_level = '2';
            }
            SHORT_OPT("O3") {
                cfg.opt_level = '3';
            }
            SHORT_OPT("Os") {
                cfg.opt_level = 's';
            }
            SHORT_OPT("g") {
                cfg.debug = true;
            }
            LONG_OPT_VAL("targets") {
                if (::strcasecmp(std::string(optval).c_str(), "all") == 0) {
                    cfg.targets = u::all_targets();
                } else {
                    cfg.targets = u::from_list_string(optval);
                }

                if (std::find(cfg.targets.begin(), cfg.targets.end(), u::target::NONE) !=
                    cfg.targets.end()) {
                    fmt::print(stderr, "invalid --target value: {}\n", optval);
                    return 1;
                }
            }
            SHORT_OPT_VAL("std") {
                cfg.cxx_std = optval;
            }
            PREFIX_OPT_VAL("I") {
                cfg.include_dirs.emplace_back(optval);
            }
            PREFIX_OPT_VAL("L") {
                cfg.library_dirs.emplace_back(optval);
            }
            PREFIX_OPT_VAL("l") {
                cfg.libraries.emplace_back(optval);
            }
            SHORT_OPT("v") {
                cfg.verbose = true;
            }
            LONG_OPT("verbose") {
                cfg.verbose = true;
            }
            SHORT_OPT("q") {
                cfg.verbose = false;
            }
            LONG_OPT("quiet") {
                cfg.verbose = false;
            }
            SHORT_OPT("save-temps") {
                cfg.save_temps = true;
            }
            SHORT_OPT("save-kernels") {
                cfg.save_kernels = true;
            }
            SHORT_OPT("save-xmls") {
                cfg.save_xmls = true;
            }
            PREFIX_OPT_VAL("D") {
                if (auto const eq = optval.find('='); eq != std::string_view::npos) {
                    std::string k(optval.substr(0, eq)), v(optval.substr(eq + 1));
                    cfg.defines.emplace(k, v);
                } else {
                    cfg.defines.emplace(std::string(optval), "");
                }
            }
            LONG_OPT_VAL("fsanitize") {
                cfg.fsanitize = optval;
            }
            SHORT_OPT("MD") {
                cfg.md = true;
            }
            SHORT_OPT("MMD") {
                cfg.mmd = true;
            }
            SHORT_OPT("MM") {
                cfg.mm = true;
            }
            SHORT_OPT("M") {
                cfg.m = true;
            }
            SHORT_OPT_VAL("MF") {
                cfg.mf = optval;
            }
            SHORT_OPT_VAL("MT") {
                cfg.mt = optval;
            }
            LONG_OPT_VAL("stop-at") {
                cfg.stop_at = optval;
            }
#ifdef CUDAToolkit
            LONG_OPT_VAL("nvcc") {
                cfg.nvcc = optval;
            }
            LONG_OPT_VAL("cudacxx") {
                cfg.nvcc = optval;
            }
            SHORT_OPT_VAL("arch") {
                cfg.cuda_arch = optval;
            }
#endif

            if (matched) {
                continue;
            }

            if (has_prefix(arg, "-")) {
                fmt::print(stderr, FMT_COMPILE("Error: unknown option: {}\n"), argv[i]);
                throw exit_failure();
            }

            cfg.inputs.emplace_back(arg);
        }

        cfg.validate();

        workflow(cfg).run();

        if (cfg.verbose) {
            auto const t_end = std::chrono::high_resolution_clock::now();
            fmt::print(FMT_COMPILE("  TOTAL: {:.2f} sec\n"), delta_sec(t_start, t_end));
        }
    } catch (std::system_error const& e) {
        fmt::print(stderr, FMT_COMPILE("{}\n"), e.what());
        return 1;
    } catch (std::exception const& e) {
        fmt::print(stderr, FMT_COMPILE("Error: {}\n"), e.what());
        return 1;
    } catch (exit_failure const&) {
        return 1;
    } catch (...) {
        fmt::print(stderr, FMT_COMPILE("Error: Got unknown exception\n"));
        throw;
    }

    return 0;
}
