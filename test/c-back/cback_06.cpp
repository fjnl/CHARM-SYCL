#include "common.hpp"

void test() {
    next_is("int test(){");

    next_is("int i;");
    next_is("for(");
    next_is("i = 0;");
    next_is("i < 100;");
    next_is("i++");
    next_is("){");
    next_is("i;");
    next_is("}");
    next_is("return i;");

    next_is("}");
    eof();
}

TEST_MAIN(test)
