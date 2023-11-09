#pragma once
#include <charm/sycl.hpp>

CHARM_SYCL_BEGIN_NAMESPACE

struct exception : public virtual std::exception {
    exception(std::error_code ec, const std::string& what_arg)
        : ec_(ec), what_(make_what(ec) + what_arg), ctx_() {}

    exception(std::error_code ec, const char* what_arg)
        : ec_(ec), what_(make_what(ec) + what_arg), ctx_() {}

    exception(std::error_code ec) : ec_(ec), what_(make_what(ec)), ctx_() {}

    exception(int ev, const std::error_category& ecat, const std::string& what_arg)
        : ec_(ev, ecat), what_(make_what(ev, ecat) + what_arg), ctx_() {}

    exception(int ev, const std::error_category& ecat, const char* what_arg)
        : ec_(ev, ecat), what_(make_what(ev, ecat) + what_arg), ctx_() {}

    exception(int ev, const std::error_category& ecat)
        : ec_(ev, ecat), what_(make_what(ev, ecat)), ctx_() {}

    exception(context ctx, std::error_code ec, const std::string& what_arg)
        : ec_(ec), what_(make_what(ec) + what_arg), ctx_(ctx) {}

    exception(context ctx, std::error_code ec, const char* what_arg)
        : ec_(ec), what_(make_what(ec) + what_arg), ctx_(ctx) {}

    exception(context ctx, std::error_code ec) : ec_(ec), what_(), ctx_(ctx) {}

    exception(context ctx, int ev, const std::error_category& ecat, const std::string& what_arg)
        : ec_(ev, ecat), what_(make_what(ev, ecat) + what_arg), ctx_(ctx) {}

    exception(context ctx, int ev, const std::error_category& ecat, const char* what_arg)
        : ec_(ev, ecat), what_(make_what(ev, ecat) + what_arg), ctx_(ctx) {}

    exception(context ctx, int ev, const std::error_category& ecat)
        : ec_(ev, ecat), what_(make_what(ev, ecat)), ctx_(ctx) {}

    const std::error_code& code() const noexcept {
        return ec_;
    }

    const std::error_category& category() const noexcept {
        return ec_.category();
    }

    const char* what() const noexcept override {
        return what_.c_str();
    }

    bool has_context() const noexcept {
        return ctx_.has_value();
    }

    context get_context() const {
        return *ctx_;
    }

private:
    static std::string make_what(int ec, std::error_category const& ecat) {
        return std::string("[") + ecat.message(ec) + "] ";
    }

    static std::string make_what(std::error_code ec) {
        return std::string("[") + ec.message() + "] ";
    }

    std::error_code ec_;
    std::string what_;
    std::optional<context> ctx_;
};

namespace detail {

struct error_category : std::error_category {
    char const* name() const noexcept override {
        return "sycl";
    }

    std::string message(int ev) const override {
        switch (static_cast<errc>(ev)) {
            case errc::success:
                return "success";
            case errc::runtime:
                return "runtime";
            case errc::kernel:
                return "kernel";
            case errc::accessor:
                return "accessor";
            case errc::nd_range:
                return "nd_range";
            case errc::event:
                return "event";
            case errc::kernel_argument:
                return "kernel_argument";
            case errc::build:
                return "build";
            case errc::invalid:
                return "invalid";
            case errc::memory_allocation:
                return "memory_allocation";
            case errc::platform:
                return "platform";
            case errc::profiling:
                return "profiling";
            case errc::feature_not_supported:
                return "feature_not_supported";
            case errc::kernel_not_supported:
                return "kernel_not_supported";
            case errc::backend_mismatch:
                return "backend_mismatch";
            case errc::unimplemented:
                return "unimplemented";
            default:
                return "unknown error";
        }
    }
};

}  // namespace detail

inline const std::error_category& sycl_category() noexcept {
    static detail::error_category ecat;
    return ecat;
}

inline std::error_code make_error_code(errc e) noexcept {
    return {static_cast<int>(e), sycl_category()};
}

CHARM_SYCL_END_NAMESPACE
