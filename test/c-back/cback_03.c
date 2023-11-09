struct B;

struct A {
    int x;
};

struct C {
    int x[10][20];
};

void test() {
    struct B* x1;
    struct A x2;
    struct A* x3;
    struct C x4;
    struct C* x5;

    x2.x;
    x3->x;
    x4.x[1][2];
    x5->x[1][2];

    x3 = (struct A*)x3;
}
