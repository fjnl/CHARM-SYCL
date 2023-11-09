#include "common.hpp"

void test() {
    next_is("void test(){");

    next_is("1;");
    next_is("1U;");
    next_is("1L;");
    next_is("1UL;");
    next_is("1ULL;");
    next_is("9223372036854775807L;");
    next_is("18446744073709551615ULL;");
    next_is("1.0;");
    next_is("1.0f;");

    next_is("-1;");
    next_is("-1U;");
    next_is("-1L;");
    next_is("-1UL;");
    next_is("-1ULL;");
    next_is("-9223372036854775807L;");
    next_is("-18446744073709551615ULL;");
    next_is("-1.0;");
    next_is("-1.0f;");

    next_is("}");
    eof();
}

TEST_MAIN(test)
