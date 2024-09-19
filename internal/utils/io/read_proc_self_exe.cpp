#include <utils/errors.hpp>
#include <utils/io.hpp>

namespace utils::io {

#if defined(HAS_NSGetExecutabePath)
#    include <mach-o/dyld.h>

boost::leaf::result<std::string> read_proc_self_exe() {
    std::vector<char> buff(4096);
    uint32_t len = 0;

    _NSGetExecutablePath(nullptr, &len);
    buff.resize(len + 1);
    _NSGetExecutablePath(buff.data(), &len);

    return std::string(buff.data());
}

#elif defined(HAS_PROC_SELF_EXE)
#    include <unistd.h>

boost::leaf::result<std::string> read_proc_self_exe() {
    std::vector<char> buff(4096);

    for (;;) {
        auto const n = readlink("/proc/self/exe", buff.data(), buff.size());

        if (n == -1) {
            return utils::errors::make_system_error("readlink",
                                                    boost::leaf::e_file_name{"/proc/self/exe"});
        }

        if (static_cast<size_t>(n) < buff.size()) {
            buff.resize(n);
            break;
        }

        buff.resize(buff.size() + 4096);
    }

    return std::string(buff.begin(), buff.end());
}

#endif

}  // namespace utils::io
