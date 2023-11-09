#include "common.hpp"

void test() {
    next_is("void test2();");
    next_is("void test(){}");
    eof();
}

TEST_MAIN(test)
