struct A {
    int x;
};

int f(int a, int b);

void test(struct A c, struct A* d, const struct A* e) {
    f(1, 2);
}
