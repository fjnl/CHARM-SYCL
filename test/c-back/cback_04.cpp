#include "common.hpp"

void test() {
    next_is("struct A{");
    next_is("int x;");
    next_is("};");

    next_is("int f(int a,int b);");

    next_is("void test(struct A c,struct A* d,const struct A* e){");
    next_is("f(1,2);");
    next_is("}");

    eof();
}

TEST_MAIN(test)
