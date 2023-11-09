#pragma once

#include <string>
#include <string_view>

namespace utils {

struct naming_utils {
    explicit naming_utils() = default;

    explicit naming_utils(size_t next_id);

    std::string gen_var();

    std::string gen_var(std::string_view name);

    std::string gen_func(std::string_view name);

    std::string gen_type(std::string_view name);

    std::string user_var(std::string_view org);

    std::string user_func(std::string_view org);

    std::string user_type(std::string_view org);

    std::string xfunc_type();

    std::string xptr_type();

    std::string xstruct_type();

    std::string xbasic_type();

    static char const* this_name();

    size_t nextid();

    size_t get_nextid() const;

    void set_nextid(size_t next_id);

private:
    size_t next_id_ = 0;
};

}  // namespace utils
