#pragma once

#include "config.hpp"

enum cc_vendor {
    gcc,
    clang,
    internal_clang,
};

boost::leaf::result<cc_vendor> check_cc_vendor(config const& cfg);
