#include "common.hpp"

void test() {
    next_is("void test(){");

    next_is("{if(1){");
    next_is("2;");
    next_is("}}");

    next_is("{if(3){");
    next_is("4;");
    next_is("}else{");
    next_is("5;");
    next_is("}}");

    next_is("}");
    eof();
}

TEST_MAIN(test)
