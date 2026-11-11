int g_n;

int ext(int v) { g_n++; return v * 2 - 1; }
double dext(double v) { g_n++; return v * 0.5 + 1.0; }
long lext(long v) { g_n++; return v * 3 + 1; }

int pressure_nocall(int n) {
    int a0 = n + 1, a1 = n * 2, a2 = n + 3, a3 = n * 4, a4 = n + 5, a5 = n * 6, a6 = n + 7, a7 = n * 8;
    int a8 = n + 9, a9 = n * 10, a10 = n + 11, a11 = n * 12, a12 = n + 13, a13 = n * 14, a14 = n + 15, a15 = n * 16;
    int a16 = n + 17, a17 = n * 18, a18 = n + 19, a19 = n * 20, a20 = n + 21, a21 = n * 22, a22 = n + 23, a23 = n * 24;
    int a24 = n + 25, a25 = n * 26, a26 = n + 27, a27 = n * 28, a28 = n + 29, a29 = n * 30;
    return a0 + a1 * 2 - a2 + a3 * 3 - a4 + a5 * 5 - a6 + a7 * 7 - a8 + a9 * 11 - a10 + a11 * 13 - a12 + a13 * 17 -
           a14 + a15 * 19 - a16 + a17 * 23 - a18 + a19 * 29 - a20 + a21 * 31 - a22 + a23 * 37 - a24 + a25 * 41 - a26 +
           a27 * 43 - a28 + a29 * 47 + (a0 ^ a29) + (a1 | a28) + (a2 & a27);
}

int pressure_calls(int n) {
    int a0 = n + 1, a1 = n * 2, a2 = n + 3, a3 = n * 4, a4 = n + 5, a5 = n * 6, a6 = n + 7, a7 = n * 8;
    int a8 = n + 9, a9 = n * 10, a10 = n + 11, a11 = n * 12, a12 = n + 13, a13 = n * 14, a14 = n + 15, a15 = n * 16;
    int a16 = n + 17, a17 = n * 18, a18 = n + 19, a19 = n * 20, a20 = n + 21, a21 = n * 22, a22 = n + 23, a23 = n * 24;
    int a24 = n + 25, a25 = n * 26, a26 = n + 27, a27 = n * 28, a28 = n + 29, a29 = n * 30;
    int c1 = ext(a0 + a15);
    int c2 = ext(a1 + a16);
    int mid = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + c1;
    int c3 = ext(mid);
    return a0 + a1 * 2 - a2 + a3 * 3 - a4 + a5 * 5 - a6 + a7 * 7 - a8 + a9 * 11 - a10 + a11 * 13 - a12 + a13 * 17 -
           a14 + a15 * 19 - a16 + a17 * 23 - a18 + a19 * 29 - a20 + a21 * 31 - a22 + a23 * 37 - a24 + a25 * 41 - a26 +
           a27 * 43 - a28 + a29 * 47 + c1 + c2 * 3 + c3 * 5 + ext(a29) + ext(a28) * ext(a27);
}

double dpressure(int n) {
    double d0 = n + 0.5, d1 = n * 1.5, d2 = n + 2.5, d3 = n * 3.5, d4 = n + 4.5, d5 = n * 5.5, d6 = n + 6.5, d7 = n * 7.5;
    double d8 = n + 8.5, d9 = n * 9.5, d10 = n + 10.5, d11 = n * 11.5, d12 = n + 12.5, d13 = n * 13.5, d14 = n + 14.5;
    double d15 = n * 15.5, d16 = n + 16.5, d17 = n * 17.5, d18 = n + 18.5, d19 = n * 19.5;
    double e = dext(d0) + dext(d19);
    return d0 + d1 * d2 - d3 + d4 * d5 - d6 + d7 * d8 - d9 + d10 * d11 - d12 + d13 * d14 - d15 + d16 * d17 - d18 +
           d19 * d0 + e + dext(d5 + d6);
}

long mixed_live(int n) {
    int i1 = n + 1, i2 = n + 2, i3 = n + 3, i4 = n + 4, i5 = n + 5, i6 = n + 6, i7 = n + 7, i8 = n + 8, i9 = n + 9, i10 = n + 10;
    long l1 = n * 10000000000L, l2 = l1 + 7, l3 = l2 * 3, l4 = l3 - 11, l5 = l4 + i1;
    double d1 = n + 0.25, d2 = d1 * 2, d3 = d2 + 1, d4 = d3 * 3, d5 = d4 - 0.5;
    float f1 = n + 0.5f, f2 = f1 * 2.0f, f3 = f2 + 1.5f;
    char c1 = (char)(n + 100), c2 = (char)(n + 120);
    long r = lext(l1) + lext(l2) + lext(i1 + i2 + i3);
    double dr = dext(d1) + dext(d2) * dext(d3);
    return r + l1 + l2 + l3 + l4 + l5 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9 + i10 + (long)(d1 + d2 + d3 + d4 + d5) +
           (long)dr + (long)(f1 + f2 + f3) + c1 + c2 + lext(l5 % 1000);
}

int fib_live(int k, int a, int b, int c) {
    if (k <= 1) return a + b + c + k;
    int x = k * 3 + a;
    int y = k * 5 + b;
    long z = k * 100000000000L + c;
    double w = k * 0.5;
    int r1 = fib_live(k - 1, a + 1, b + 2, c + 3);
    int r2 = fib_live(k - 2, b, c, a);
    return r1 + r2 + x + y + (int)(z % 1000) + (int)w;
}

int ack(int m, int n) {
    if (m == 0) return n + 1;
    if (n == 0) return ack(m - 1, 1);
    return ack(m - 1, ack(m, n - 1));
}

long sum_tree(int depth, long acc, double scale, char tag) {
    if (depth == 0) return acc + tag;
    long l = sum_tree(depth - 1, acc + depth, scale * 2, tag + 1);
    long r = sum_tree(depth - 1, acc * 2, scale + 1, tag - 1);
    return l - r + (long)scale + tag;
}

int stat_rec(int k) {
    static int calls;
    static long acc = 5;
    calls++;
    acc = acc * 3 + k;
    if (k > 0) stat_rec(k - 1);
    return (int)(acc % 100000) + calls;
}

int main(int n) {
    int p1 = pressure_nocall(n);
    int p2 = pressure_calls(n);
    double p3 = dpressure(n);
    long p4 = mixed_live(n);
    int p5 = fib_live(12, n, n + 1, n + 2);
    int p6 = ack(2, 3 + n);
    long p7 = sum_tree(8, n, 1.5, 10);
    int p8 = stat_rec(6 + n);
    printf("%d %d %f %ld\n", p1, p2, p3, p4);
    printf("%d %d %ld %d %d\n", p5, p6, p7, p8, g_n);
    return p1 + p2 + (int)p3 + (int)(p4 % 1000) + p5 + p6 + (int)p7 + p8 + g_n;
}
