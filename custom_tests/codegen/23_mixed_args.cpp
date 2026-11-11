struct P3 { char a; char b; char c; };
struct D2 { double x; double y; };
struct Blob { long v[6]; };

long mix1(float f1, double d1, int i1, long l1, char c1, unsigned int u1, int* p1, float f2, double d2,
          int i2, long l2, char c2, unsigned int u2, int* p2, float f3, double d3, short s3) {
    return (long)(f1 * 10 + d1 * 100 + f2 * 1000 + d2 * 10000 + f3 * 100000 + d3 * 1000000) + i1 + l1 * 3 + c1 +
           u1 + *p1 + i2 + l2 * 5 + c2 + u2 + *p2 * 2 + s3;
}

double mix2(int a, P3 t, double b, Blob bl, int c, D2 d, float e, int f, int g, int h, int i, D2 j, long k,
            unsigned char l) {
    double r = a + t.a * 2 + t.b * 3 + t.c * 4 + b + bl.v[0] + bl.v[5] * 2 + c + d.x + d.y * 2 + e + f + g + h +
               i + j.x * 3 + j.y * 5 + k + l;
    return r;
}

float fl10(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j,
           float k, float l) {
    return a - b + c * d - e + f - g + h * i - j + k * l;
}

double db10(double a, double b, double c, double d, double e, double f, double g, double h, double i,
            double j, int k, double l) {
    return a - b + c * d - e + f - g + h * i - j + k * l;
}

unsigned int uargs(unsigned int a, unsigned char b, unsigned short c, int d, int e, int f, int g, int h,
                   unsigned int i, unsigned char j, unsigned short k, char l, short m) {
    return a + b * 3 + c * 5 + d + e + f + g + h + i * 7 + j * 11 + k * 13 + l + m;
}

long relay(int a, double b, int c, double d, int e, double f, int g, double h, int i, double j, int k, double l) {
    return (long)(a + b + c + d + e + f + g + h + i + j + k + l) + (long)db10(b, d, f, h, j, l, b, d, f, h, a, b);
}

int main(int n) {
    int x1 = n + 1, x2 = n + 2;
    P3 t;
    t.a = (char)n; t.b = (char)(n + 1); t.c = (char)(n + 2);
    Blob bl;
    for (int i = 0; i < 6; i++) bl.v[i] = i * 1000000000L + n;
    D2 d, j;
    d.x = n + 0.5; d.y = n * 2.0;
    j.x = n - 0.25; j.y = 3.0;
    printf("%ld\n", mix1(1.5f, 2.25, 3, 4000000000L, 'a', 4000000000U, &x1, 6.5f, 7.75, 8, -9000000000L, 'b',
                         3000000000U, &x2, 0.5f, 0.125, (short)-300));
    printf("%f\n", mix2(1, t, 2.5, bl, 3, d, 4.5f, 5, 6, 7, 8, j, 9000000000L, 250));
    printf("%f %f\n", (double)fl10(1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f, 9.5f, 10.5f, 11.5f, 12.5f + n),
           db10(1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11, 12.5 + n));
    printf("%u\n", uargs(4000000000U, 250, 65000, -1, -2, -3, -4, -5, 3000000000U, 255, 65535, -7, -300));
    printf("%ld\n", relay(1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6 + n, 6.5));
    printf("%d %f %ld %f %s %c %u %lu\n", n, 1.5, 3000000000L, 2.5, "str", 'x', 4000000000U, 18446744073709551615UL);
    printf("%d %d %d %d %d %d %d %d %f %f %f %d %f\n", 1, 2, 3, 4, 5, 6, 7, 8, 9.5, 10.5, 11.5, 12, 13.5);
    printf("%f %f %f %f %f %f %f %f %f %f %f %f\n", 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5);
    printf("%d %s %f %ld %c %f %d %lu %f %s %d %f\n", 1, "a", 2.5, 3L, 'c', 4.5, 5, 6UL, 7.5, "b", 8, 9.5);
    printf("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", n, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18);
    printf("%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n", (double)(float)1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1);
    return (int)(mix1(1.5f, 2.25, 3, 40L, 'a', 4U, &x1, 6.5f, 7.75, 8, -90L, 'b', 3U, &x2, 0.5f, 0.125, (short)-300) % 1000) +
           (int)uargs(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, n);
}
