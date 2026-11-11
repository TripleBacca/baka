int g_calls;

int touch(int v) {
    g_calls++;
    return v * 3 + 1;
}

double dtouch(double v) {
    g_calls++;
    return v * 1.5 + 0.25;
}

int main(int n) {
    int a = n + 1, b = n + 2, c = n + 3, d = n + 4, e = n + 5, f = n + 6, g = n + 7, h = n + 8;
    int i = n + 9, j = n + 10, k = n + 11, l = n + 12, m = n + 13, o = n + 14, p = n + 15, q = n + 16;
    double x1 = 1.5, x2 = 2.5, x3 = 3.5, x4 = 4.5, x5 = 5.5, x6 = 6.5, x7 = 7.5, x8 = 8.5, x9 = 9.5, x10 = 10.5;
    long big1 = 100000000000L, big2 = 200000000000L, big3 = 300000000000L;
    int t1 = touch(a) + touch(b) * touch(c);
    double y1 = dtouch(x1) + dtouch(x2) * dtouch(x3);
    int sum = a + b + c + d + e + f + g + h + i + j + k + l + m + o + p + q;
    int mix = (a * b + c * d) - (e * f - g * h) + (i * j + k * l) * (m - o + p - q);
    double dsum = x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10;
    double dmix = (x1 * x2 + x3 * x4) * (x5 - x6) / (x7 + x8 * x9) - x10;
    long lsum = big1 + big2 * 2 - big3 + a + b;
    int after = touch(sum) + touch(mix % 1000) + (int)dtouch(dsum);
    printf("%d %d %f %d\n", t1, sum, y1, mix);
    printf("%f %f %ld %d %d\n", dsum, dmix, lsum, after, g_calls);
    return t1 + sum + mix + after + (int)(lsum % 1000) + (int)y1 + g_calls;
}
