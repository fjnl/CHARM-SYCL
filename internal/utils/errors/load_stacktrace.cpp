#include <boost/stacktrace.hpp>
#include <utils/errors.hpp>

namespace utils::errors {

void load_backtrace(e_backtrace& e) {
    e.msg = boost::stacktrace::to_string(boost::stacktrace::stacktrace());
}

}  // namespace utils::errors
