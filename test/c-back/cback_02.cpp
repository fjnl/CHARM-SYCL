#include "common.hpp"

void test() {
    next_is("void test(int a){");

    next_is("int* b;");
    next_is("a = a << a;");
    next_is("a = a >> a;");
    next_is("a = a + a;");
    next_is("a = a - a;");
    next_is("a = a * a;");
    next_is("a = a / a;");
    next_is("a = a % a;");
    next_is("a = a == a;");
    next_is("a = a != a;");
    next_is("a = a >= a;");
    next_is("a = a > a;");
    next_is("a = a <= a;");
    next_is("a = a < a;");
    next_is("a = a && a;");
    next_is("a = a || a;");
    next_is("a = a & a;");
    next_is("a = a | a;");
    next_is("a = a ^ a;");
    next_is("a += a;");
    next_is("a -= a;");
    next_is("a *= a;");
    next_is("a /= a;");
    next_is("a <<= a;");
    next_is("a >>= a;");
    next_is("a &= a;");
    next_is("a |= a;");
    next_is("a ^= a;");
    next_is("a;");
    next_is("++a;");
    next_is("a++;");
    next_is("-a;");
    next_is("--a;");
    next_is("a--;");
    next_is("!a;");
    next_is("~a;");
    next_is("&a;");
    next_is("*b;");
    next_is("a = (int)a;");
    next_is("a = (int*)a;");
    next_is("a = (const int*)a;");

    next_is("}");
    eof();
}

TEST_MAIN(test)
