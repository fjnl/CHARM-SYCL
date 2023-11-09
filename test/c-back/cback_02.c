void test(int a) {
    int* b;

    a = a << a;
    a = a >> a;
    a = a + a;
    a = a - a;
    a = a * a;
    a = a / a;
    a = a % a;
    a = a == a;
    a = a != a;
    a = a >= a;
    a = a > a;
    a = a <= a;
    a = a < a;
    a = a && a;
    a = a || a;
    a = a & a;
    a = a | a;
    a = a ^ a;
    a += a;
    a -= a;
    a *= a;
    a /= a;
    a <<= a;
    a >>= a;
    a &= a;
    a |= a;
    a ^= a;
    a;
    ++a;
    a++;
    -a;
    --a;
    a--;
    !a;
    ~a;
    &a;
    *b;

    a = (int)a;
    a = (int*)a;
    a = (const int*)a;
}
