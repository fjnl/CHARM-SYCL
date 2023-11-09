#include "common.hpp"

void test() {
    next_is("struct B;");

    next_is("struct A{");
    next_is("int x;");
    next_is("};");

    next_is("struct C{");
    next_is("int x[10][20];");
    next_is("};");

    next_is("void test(){");

    next_is("struct B* x1;");
    next_is("struct A x2;");
    next_is("struct A* x3;");
    next_is("struct C x4;");
    next_is("struct C* x5;");

    next_is("x2.x;");
    next_is("x3->x;");
    next_is("*(*(x4.x + 1) + 2);");
    next_is("*(*(x5->x + 1) + 2);");

    next_is("x3 = (struct A*)x3;");

    next_is("}");

    eof();
}

TEST_MAIN(test)
