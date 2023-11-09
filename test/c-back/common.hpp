#pragma once

#include <fstream>
#include <iterator>
#include <regex>
#include <assert.h>

namespace {

std::string input;
std::string::const_iterator it, end;

inline void next_is(std::string_view str) {
    while (it != end && std::isspace(*it)) {
        ++it;
    }

    if (std::distance(it, end) >= static_cast<ssize_t>(str.size())) {
        auto const p = std::next(it, str.size());

        if (std::equal(it, p, str.begin(), str.end())) {
            it = p;
            return;
        }
    }

    fprintf(stderr, "Error: expected but not found: %s\n", std::string(str).c_str());
    exit(1);
}

inline void eof() {
    while (it != end && std::isspace(*it)) {
        ++it;
    }
    assert(it == end && "not at end-of-file");
}

}  // namespace

#define TEST_MAIN(func)                                         \
    int main(int argc, char** argv) {                           \
        if (argc != 2) {                                        \
            return 1;                                           \
        }                                                       \
                                                                \
        std::ifstream ifs;                                      \
        ifs.exceptions(ifs.failbit | ifs.badbit);               \
        ifs.open(argv[1]);                                      \
                                                                \
        std::istreambuf_iterator<char> ifs_begin(ifs), ifs_end; \
        input.assign(ifs_begin, ifs_end);                       \
        it = input.cbegin();                                    \
        end = input.cend();                                     \
                                                                \
        func();                                                 \
                                                                \
        return 0;                                               \
    }
