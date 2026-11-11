int ten(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8 + i * 9 + j * 10;
}

long eleven(long a, long b, long c, long d, long e, long f, long g, long h, long i, long j, long k) {
    return a - b + c - d + e - f + g - h + i - j + k * 100;
}

double mixed(int a, double b, int c, double d, int e, double f, int g, double h, int i, double j, int k,
             double l, int m, double o) {
    return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8 + i * 9 + j * 10 + k * 11 + l * 12 +
           m * 13 + o * 14;
}

char narrow(char a, short b, unsigned char c, unsigned short d, int e, char f, short g, unsigned char h,
            char i, short j, unsigned char k) {
    printf("%d %d %d %d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h, i, j, k);
    return a + b + c + d + e + f + g + h + i + j + k;
}

float singles(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j) {
    return a + b + c + d + e + f + g + h + i + j;
}

int relay(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    return ten(j, i, h, g, f, e, d, c, b, a) + ten(a, b, c, d, e, f, g, h, i, j);
}

int main(int n) {
    int t = ten(1, 2, 3, 4, 5, 6, 7, 8, 9, n);
    long e = eleven(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, n);
    double m = mixed(1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6, 6.5, 7, n + 0.25);
    printf("%d %ld %f\n", t, e, m);
    int w = narrow(-1, -2, 250, 65000, n, -3, -300, 200, 127, -128, 255);
    float s = singles(1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f, 9.5f, 10.5f + n);
    printf("%d %f\n", w, (double)s);
    int r = relay(1, 2, 3, 4, 5, 6, 7, 8, 9, 10 + n);
    return t + (int)e + (int)m + r + (int)s;
}
