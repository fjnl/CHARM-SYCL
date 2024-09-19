#pragma once

#include "chsy-lower.hpp"

void add_replace_math_func(replace_builtin_map_t& map, char const* name, char const* float_,
                           char const* double_);

void add_common_replace_math_funcs(replace_builtin_map_t&, implement_builtin_map_t&);
