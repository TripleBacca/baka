int cnt;

double mknan(int n) {
    double z = n * 0.0;
    return z / z;
}

int cmp_branch(double x, double y) {
    int r = 0;
    if (x < y) r |= 1;
    if (x <= y) r |= 2;
    if (x > y) r |= 4;
    if (x >= y) r |= 8;
    if (x == y) r |= 16;
    if (x != y) r |= 32;
    if (!(x < y)) r |= 64;
    if (!(x == y)) r |= 128;
    return r;
}

int cmp_value(double x, double y) {
    bool a = x < y;
    bool b = x <= y;
    bool c = x > y;
    bool d = x >= y;
    bool e = x == y;
    bool f = x != y;
    bool g = !(x < y);
    return a + b * 2 + c * 4 + d * 8 + e * 16 + f * 32 + g * 64;
}

int cmp_float(float x, float y) {
    bool a = x < y;
    bool b = x == y;
    bool c = x != y;
    int r = a + b * 2 + c * 4;
    if (x >= y) r += 8;
    if (x > y || x < y) r += 16;
    return r;
}

int loop_nan(double x, int lim) {
    int k = 0;
    while (x < 10.0 && k < lim) { k++; x += 1.0; }
    return k;
}

int logic(double x, double y, int t) {
    int r = 0;
    if (x < y && t > 0) r += 1;
    if (x < y || t > 0) r += 2;
    if (!(x >= y) && !(x <= y)) r += 4;
    r += (x == x) ? 8 : 16;
    r += (x != x) ? 32 : 64;
    return r;
}

int main(int n) {
    double nan = mknan(n);
    double one = 1.0 + n;
    double zero = 0.0 * n;
    double nzero = -zero;
    float fnan = (float)nan;
    float fone = (float)one;
    int r1 = cmp_branch(nan, one);
    int r2 = cmp_branch(one, nan);
    int r3 = cmp_branch(nan, nan);
    int r4 = cmp_branch(one, one + 1.0);
    int r5 = cmp_branch(zero, nzero);
    int v1 = cmp_value(nan, one);
    int v2 = cmp_value(one, nan);
    int v3 = cmp_value(nan, nan);
    int v4 = cmp_value(one, one);
    int v5 = cmp_value(nzero, zero);
    int f1 = cmp_float(fnan, fone);
    int f2 = cmp_float(fone, fnan);
    int f3 = cmp_float(fone, fone);
    int f4 = cmp_float(fnan, fnan);
    int l1 = loop_nan(nan, 5);
    int l2 = loop_nan(1.0, 100);
    int g1 = logic(nan, one, n + 1);
    int g2 = logic(one, one + 1.0, n + 1);
    int g3 = logic(one, nan, 0);
    printf("%d %d %d %d %d\n", r1, r2, r3, r4, r5);
    printf("%d %d %d %d %d\n", v1, v2, v3, v4, v5);
    printf("%d %d %d %d\n", f1, f2, f3, f4);
    printf("%d %d %d %d %d\n", l1, l2, g1, g2, g3);
    bool inline_nan = nan < one;
    bool inline_ge = nan >= one;
    bool nn = nan != nan;
    printf("%d %d %d\n", inline_nan, inline_ge, nn);
    return r1 + r2 + r3 + r4 + r5 + v1 + v2 + v3 + v4 + v5 + f1 + f2 + f3 + f4 + l1 + l2 + g1 + g2 + g3;
}
