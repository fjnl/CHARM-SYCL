#include <fmt/compile.h>
#include <fmt/format.h>
#include <utils/naming.hpp>

namespace utils {

naming_utils::naming_utils(size_t next_id) : next_id_(next_id) {}

std::string naming_utils::gen_var() {
    return fmt::format(FMT_COMPILE("_s__{}"), nextid());
}

std::string naming_utils::gen_var(std::string_view name) {
    return fmt::format(FMT_COMPILE("_s__{}_{}"), name, nextid());
}

std::string naming_utils::gen_func(std::string_view name) {
    return fmt::format(FMT_COMPILE("_sF__{}_{}"), name, nextid());
}

std::string naming_utils::gen_type(std::string_view name) {
    return fmt::format(FMT_COMPILE("_sT__{}_{}"), name, nextid());
}

std::string naming_utils::user_var(std::string_view org) {
    return fmt::format(FMT_COMPILE("_s_{}_{}"), org, nextid());
}

std::string naming_utils::user_func(std::string_view org) {
    return fmt::format(FMT_COMPILE("_sF_{}_{}"), org, nextid());
}

std::string naming_utils::user_type(std::string_view org) {
    return fmt::format(FMT_COMPILE("_sT_{}_{}"), org, nextid());
}

std::string naming_utils::xfunc_type() {
    return fmt::format(FMT_COMPILE("F{}"), nextid());
}

std::string naming_utils::xptr_type() {
    return fmt::format(FMT_COMPILE("P{}"), nextid());
}

std::string naming_utils::xstruct_type() {
    return fmt::format(FMT_COMPILE("S{}"), nextid());
}

std::string naming_utils::xbasic_type() {
    return fmt::format(FMT_COMPILE("B{}"), nextid());
}

char const* naming_utils::this_name() {
    return "_s__this";
}

size_t naming_utils::nextid() {
    return next_id_++;
}

size_t naming_utils::get_nextid() const {
    return next_id_;
}

void naming_utils::set_nextid(size_t next_id) {
    next_id_ = next_id;
}

}  // namespace utils
