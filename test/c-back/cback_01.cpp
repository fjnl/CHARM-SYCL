#include "common.hpp"

void test() {
    next_is("void test");
    next_is("(");
    next_is("int x1");
    next_is(")");
    next_is("{");

    next_is("int x2;");
    next_is("int* x3;");
    next_is("const int* x4;");
    next_is("int x5[10];");
    next_is("int x6[10][20];");
    next_is("int x7[10][20][30];");

    next_is("char x9;");
    next_is("unsigned char x10;");
    next_is("short x11;");
    next_is("unsigned short x12;");
    next_is("int x13;");
    next_is("unsigned int x14;");
    next_is("long x15;");
    next_is("unsigned long x16;");
    next_is("long long x17;");
    next_is("unsigned long long x18;");
    next_is("float x19;");
    next_is("double x20;");

    next_is("}");
    eof();
}

TEST_MAIN(test)
