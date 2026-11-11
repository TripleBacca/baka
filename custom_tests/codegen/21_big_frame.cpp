int g_sink;

int touch(int v) {
    g_sink += v;
    return v + 7;
}

double dtouch(double v) {
    g_sink++;
    return v * 2.0;
}

int fill(int* p, int count, int seed) {
    int s = 0;
    for (int i = 0; i < count; i++) {
        p[i] = seed + i * 3;
        s += p[i] & 255;
    }
    return s;
}

int big_after(int n) {
    int a = n + 1;
    long b = n * 100000000000L;
    double c = n + 0.5;
    char d = (char)(n + 65);
    int big[10000];
    int e = touch(a);
    int s = fill(big, 10000, n);
    short f = (short)(n - 3);
    double g = dtouch(c);
    big[9999] = e;
    big[0] = (int)g;
    int h = touch(big[9999] + big[0]);
    unsigned char big2[40000];
    for (int i = 0; i < 40000; i++) big2[i] = (unsigned char)(i * 7 + n);
    int t = big2[39999] + big2[12345] + big2[0];
    long total = b + a + e + s + f + h + d + t + big[5000] + big[1];
    printf("%d %ld %f %d %d %d %d %d\n", a, b, c, d, e, s, f, h);
    printf("%f %d %ld\n", g, t, total);
    return (int)(total % 100000);
}

double big_double(int n) {
    double arr[5000];
    double acc = n;
    for (int i = 0; i < 5000; i++) arr[i] = i * 0.5 + n;
    for (int i = 0; i < 5000; i += 7) acc += arr[i];
    float fl = (float)n * 1.5f;
    long big_idx = 4999;
    return acc + arr[big_idx] + fl;
}

int rec(int depth, int n) {
    int local[3000];
    local[0] = n;
    local[2999] = depth;
    if (depth == 0) return local[0] + local[2999];
    int r = rec(depth - 1, n + 1);
    return r + local[0] + local[2999];
}

int many_locals(int n) {
    int a1 = n + 1, a2 = n + 2, a3 = n + 3, a4 = n + 4, a5 = n + 5, a6 = n + 6, a7 = n + 7, a8 = n + 8;
    int b1 = a1 * 2, b2 = a2 * 3, b3 = a3 * 4, b4 = a4 * 5, b5 = a5 * 6, b6 = a6 * 7, b7 = a7 * 8, b8 = a8 * 9;
    long c1 = a1 * 1000000007L, c2 = a2 * 3L, c3 = b3 * 5L, c4 = b4 * 7L;
    double d1 = a1 * 0.5, d2 = a2 * 0.25, d3 = b3 * 0.125, d4 = b4 * 2.5;
    char buf[100];
    for (int i = 0; i < 100; i++) buf[i] = (char)(i + n);
    int t1 = touch(a1) + touch(b2);
    double t2 = dtouch(d1) + dtouch(d4);
    int sum = a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + b1 + b2 + b3 + b4 + b5 + b6 + b7 + b8;
    long lsum = c1 + c2 + c3 + c4;
    double dsum = d1 + d2 + d3 + d4;
    printf("%d %d %ld %f %f %d\n", t1, sum, lsum, dsum, t2, buf[99]);
    return t1 + sum + (int)(lsum % 1000) + (int)dsum + (int)t2 + buf[50];
}

int main(int n) {
    int r1 = big_after(n);
    double r2 = big_double(n);
    int r3 = rec(5, n);
    int r4 = many_locals(n);
    printf("%d %f %d %d %d\n", r1, r2, r3, r4, g_sink);
    return r1 + (int)r2 + r3 + r4 + g_sink;
}
