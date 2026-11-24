int g_sink;
int f2i(double d) { if (!(d > -1000000000.0 && d < 1000000000.0)) return 0; return (int)d; }
unsigned int f2u(double d) { if (!(d > -0.5 && d < 4000000000.0)) return 0; return (unsigned int)d; }
long f2l(double d) { if (!(d > -4000000000000000000.0 && d < 4000000000000000000.0)) return 0; return (long)d; }
unsigned long f2ul(double d) { if (!(d > -0.5 && d < 18000000000000000000.0)) return 0; return (unsigned long)d; }
double san(double d) { if (d != d) return 12345.0; if (d > 1e30) return 1e30; if (d < -1e30) return -1e30; return d; }
long sink(long v) { g_sink += (int)v; return v + 1; }
int h0(int a, long b) { return a * 3 + (int)b; }
long h1(int a, long b) { return b - a; }
int h2(int a, long b) { g_sink++; return (int)(b ^ a); }

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    unsigned int v0 = (unsigned int)(n + 1);
    unsigned short v1 = (unsigned short)(n + 8);
    unsigned long v2 = (unsigned long)(n + 15);
    float v3 = (float)(n * 0.5 + 3.25);
    unsigned long v4 = (unsigned long)(n + 29);
    unsigned long v5 = (unsigned long)(n + 36);
    unsigned char v6 = (unsigned char)(n + 43);
    int v7 = (int)(n + 50);
    unsigned char v8 = (unsigned char)(n + 57);
    char v9 = (char)(n + 64);
    float v10 = (float)(n * 0.5 + 10.25);
    char v11 = (char)(n + 78);
    double v12 = (double)(n * 0.5 + 12.25);
    long v13 = (long)(n + 92);
    double v14 = (double)(n * 0.5 + 14.25);
    char v15 = (char)(n + 106);
    double v16 = (double)(n * 0.5 + 16.25);
    float v17 = (float)(n * 0.5 + 17.25);
    long v18 = (long)(n + 127);
    double v19 = (double)(n * 0.5 + 19.25);
    double v20 = (double)(n * 0.5 + 20.25);
    int v21 = (int)(n + 148);
    float v22 = (float)(n * 0.5 + 22.25);
    float v23 = (float)(n * 0.5 + 23.25);
    v23 = ((- (float)(v15)) - ((float)(v9) + (float)(v13)));
    if (3 == (char)(v2)) {
        v21 = f2i(v16);
    } else {
        v16 = (double)((long)(v7));
    }
    if ((((unsigned short)f2i(v3) & 1) >= ((unsigned short)(v7) + (unsigned short)(v4))) || ((65535 * f2ul(v23)) >= (9223372036854775808UL + f2ul(v12)))) {
        if ((f2l(v10) <= (v18 & (long)(v9))) || (!(0 >= (char)(v8)))) {
            v10 = ((float)(((char)(v21) ^ (char)f2i(v23))) + (float)(v0));
            v18 = (long)(f2u(v23));
            v2 = v5;
        } else {
            v8 &= (unsigned char)h0(((~ (int)(v15)) + f2i(v23)), (long)(v1));
            v12 = ((double)(v4) + ((100.0 * v14) + ((double)(v0) - (double)(v3))));
        }
        v23 = (((char)(v8) * (((char)(v1) << (2147483647 & 7)) | (char)f2i(v20))) > ((((2 + 255) > ((unsigned int)(v21) + (unsigned int)(v11))) && (!(12345 > (unsigned char)(v13)))) ? 65536 : (((char)f2i(v22) & v9) ^ (1000 ^ 1000)))) ? ((float)(v5) + (1.25 * 2.0)) : (((unsigned int)(v21) >> (1 & 7)) >= (~ ((unsigned int)(v13) % (((unsigned int)(v6) & 15) + 1)))) ? (float)(((int)(v8) ^ (int)(v1))) : (float)(v0);
        if (((v9 > (char)(v4)) && (1)) && ((0) && (1000 <= 3))) {
            v8 = ((unsigned char)f2i((double)(v3)) * ((12345 * ((f2ul(v23) != 5) ? (unsigned char)f2i(v12) : 256)) % ((((unsigned char)f2i(v16) * (unsigned char)(v4)) & 15) + 1)));
            v19 = (double)(v1);
        }
    }
    v14 = ((double)(v23) - (double)(((f2l(v22) + (long)(v21)) & (f2l(v10) | (long)(v5)))));
    v19 = ((((0.5 - 2.0) - (double)(v5)) - 2.0) - (((v19 / 3.0) - ((double)(v17) - 0.0)) != ((100.0 * (double)(v1)) - (0.5 - (double)(v7)))) ? (v20 * (v19 - 0.5)) : (((double)(v0) * (double)(v17)) / 3.0));
    v19 = (-3.75 * ((double)(v23) > (double)(v9)) ? v14 : (double)(v4));
    if ((unsigned short)h2((f2i(v14) & f2i(v20)), (long)(1)) != 12345) {
        v7 = (((h1(f2i(v14), f2l(v23)) >> ((v7 * 65536) & 7)) * (int)(((unsigned int)(v15) >> (12345 & 7)))) << (((int)((char)(v2)) - 7) & 7));
        v15 = ((~ (12345 / (((char)(v7) & 15) + 1))) - ((char)(((float)(v0) < v23)) << ((f2i(v16) | f2i(v12)) & 7)));
        v8 = ((((1) ? (unsigned char)(v5) : (unsigned char)f2i(v14)) + ((unsigned char)f2i(v10) | 65535)) | (((unsigned char)f2i(v10) ^ 65535) - (unsigned char)((1))));
    } else {
        v14 = (double)sink(((- (f2l(v10) ^ (long)(v2)))) % 100000);
        v8 = (~ 12345);
    }
    v15 *= (char)h0(((100 * 2147483647) - (f2i(v16) & 255)), (long)(((f2i(v19) / ((f2i(v3) & 15) + 1)) > ((int)(v1) + 100))));
    if (v20 > 2.0) {
        v9 = ((char)f2i(v22) >> (((int)(v8) + f2i(v14)) & 7));
    } else {
        if ((((short)f2i(v17) / (((short)(v7) & 15) + 1)) != (- (short)(v7))) || (!(f2u(v23) > (unsigned int)(v2)))) {
            v10 = ((unsigned long)h1((int)(v15), f2l(v14)) <= f2ul(v22)) ? (100.0 + 1.25) : ((float)(v8) * 0.5);
        }
        v22 = (float)sink(((-1 % (((1000 + v13) & 15) + 1))) % 100000);
        v13 = ((f2l(v20) >= ((h1((int)(v9), f2l(v17)) & h2(v21, f2l(v20))) % (((((f2i(v19) > (int)(v11)) ? f2l(v19) : f2l(v17)) ^ f2l(v22)) & 15) + 1))) ? (((v13 & f2l(v14)) | (f2l(v3) ^ (long)(v4))) + 5000000000L) : f2l(v16));
    }
    for (int k178337 = 0; k178337 < 3; k178337++) {
        v4 = (65535 * (unsigned long)(((int)(v6) > (int)(v2))));
        v21 = (int)sink(((long)(v6)) % 100000);
    }
    v12 = ((f2l(v3) != h0(((int)(v11) + (int)(v5)), ((long)(v11) & f2l(v23)))) ? (double)(v21) : (0.0 + (2.0 - -3.75)) - (((double)(v5) + -3.75) / 3.0));
    v11 = (char)f2i(v10);
    v21 = (256 + (((v21 ^ f2i(v16)) * h0((int)(v9), -1)) | (int)(1)));
    v1 = (unsigned short)sink(((long)(((unsigned short)(v4) / (((unsigned short)(v0) & 15) + 1)))) % 100000);
    v15 = (~ (((char)f2i(v23) + (char)f2i(v19)) + (char)((1))));
    v14 = (double)(v23);
    if ((((char)(v8) + (char)(v2)) > ((char)(v8) | (char)(v6))) && ((f2l(v22) & (long)(v5)) > ((long)(v5) ^ f2l(v14)))) {
        if (!(((unsigned char)(v5) - (unsigned char)(v18)) == (- (unsigned char)f2i(v12)))) {
            v12 = (((-3.75 / 3.0) - ((double)(v11) - 2.0)) - (double)(v15));
            v8 = (unsigned char)(((short)f2i(v22) - (short)f2i(v17)));
        }
        v7 = (~ (- (int)((1000 == (char)f2i(v20)))));
        v14 = (double)sink((f2l(((float)(v21) / 3.0))) % 100000);
    } else {
        v20 = ((double)((((short)(v7) >> ((int)(v11) & 7)) & (short)f2i(v16))) + (((v16 + (double)(v18)) / 3.0) - (- (1.25 + (double)(v10)))));
        v16 = 0.0;
        v1 = (unsigned short)((f2ul(v17) == (~ f2ul(v10))));
    }
    v2 = ((((~ f2ul(v23)) | (unsigned long)(v8)) + ((9223372036854775808UL + f2ul(v20)) & (f2ul(v10) ^ f2ul(v12)))) << ((- ((int)(v8) - ((int)(v4) * f2i(v17)))) & 7));
    v13 *= v13;
    v21 = f2i(v12);
    v21 += (((f2i(v17) % ((f2i(v3) & 15) + 1)) + h1(2147483647, f2l(v3))) << ((f2i((float)(v14)) << ((int)(v2) & 7)) & 7));
    v10 = ((((unsigned char)(v9) % (((unsigned char)f2i(v3) & 15) + 1)) & (255 ^ (unsigned char)(v21))) == (((unsigned char)f2i(v17) % ((v8 & 15) + 1)) >> (65535 & 7))) ? ((float)(v9) / 3.0) : ((unsigned short)h2(-1, (long)(v7)) == (unsigned short)f2i(v3)) ? (v17 + (float)(v21)) : ((float)(v13) * (float)(v15));
    for (int k343029 = 0; k343029 < 2; k343029++) {
        v2 = (unsigned long)sink((((((double)(v23) - 100.0) == ((double)(v18) / 3.0)) ? (long)((1)) : (long)(v0))) % 100000);
    }
    v9 = 65536;
    if ((unsigned long)(v7) >= (((1.25 - -3.75) < ((float)(v9) + 0.0)) ? ((unsigned long)(v9) | (unsigned long)(v21)) : f2ul((float)(v7)))) {
        v17 = ((0.0 / 3.0) / 3.0);
        v14 = (!(f2i(v17) != (int)((((char)f2i(v12) & (char)f2i(v3)) >= ((char)(v1) + (char)(v21)))))) ? ((((double)(v1) - v19) - ((double)(v0) + 2.0)) / 3.0) : (double)((unsigned short)h2((~ v21), (long)((f2ul(v19) >= f2ul(v14)))));
    }
    v10 = ((0.5 * 0.0) - v3);
    v16 = ((((1) && ((short)(v1) < (short)f2i(v23))) ? ((double)(v23) - 0.0) : ((double)(v0) + v16) + ((v16 * (double)(v7)) + ((double)(v23) - (double)(v4)))) / 3.0);
    for (int k113346 = 0; k113346 < 4; k113346++) {
        v3 = ((((double)(v9) * (double)(v8)) == (double)(v8)) && (!(0))) ? ((2.0 - (float)(v5)) + ((float)(v0) + (float)(v18))) : ((float)(v1) - ((float)(v12) * (float)(v5)));
        v3 = ((1) && (1)) ? (0.0 * (float)(v0)) : (- (float)(v12));
    }
    v20 = (((double)(v9) * (double)(v5)) / 3.0);
    if (((1 <= (unsigned long)(v15)) && ((char)(v8) == (char)(v13))) && (!(0))) {
        v11 = (char)f2i(v10);
    }
    if (((2 % ((f2l(v17) & 15) + 1)) >> ((- f2i(v10)) & 7)) != ((- (long)(v21)) ^ ((long)(v6) * f2l(v20)))) {
        v7 = ((((unsigned short)f2i(v3) != (unsigned short)(v7)) ? f2i(v19) : f2i(v19)) - (f2i(v14) | -5));
        v0 &= ((((float)(2) + v17) >= ((unsigned int)((1000 < (short)(v8))) >= (unsigned int)h0(f2i(v23), 2)) ? (v22 * (float)(v6)) : (float)(v21)) ? (unsigned int)(1) : (((unsigned int)(v2) + 0) % (((unsigned int)((v3 != (float)(v21))) & 15) + 1)));
        for (int k908767 = 0; k908767 < 4; k908767++) {
            v4 ^= (unsigned long)(((f2i(v23) | -1) & (~ v7)));
        }
    } else {
        v5 |= ((!(((unsigned long)(v21) == 100) || ((double)(v5) == (double)(v1)))) ? ((((unsigned char)f2i(v20) <= (unsigned char)(v0)) ? 1000 : f2ul(v20)) >> ((- f2i(v22)) & 7)) : ((5 * f2ul(v16)) | (9223372036854775808UL - 0)));
        v19 = (double)sink((f2l(v14)) % 100000);
        v3 = (float)((int)(v2));
    }
    if ((((char)(v4) - (char)(v5)) % ((((char)(v13) & 65535) & 15) + 1)) <= -1) {
        v21 = ((3 / (((int)(v15) & 15) + 1)) & h2(f2i(v22), f2l(v17)));
        v7 = (((((long)(12345) <= (long)((short)(v9))) ? (int)(v11) : (f2i(v17) * (int)(v5))) - (3 - (int)((long)(v1)))) - (((~ (int)(v0)) & (int)((unsigned short)(v21))) ^ (int)(v15)));
        v18 = (long)sink((((f2l(v14) + (long)(v21)) & ((long)(v15) >> (f2i(v23) & 7)))) % 100000);
    } else {
        v18 = (((unsigned long)((((char)((0.0 < v20)) + (char)(v18)) >= (char)f2i((255 < (char)(v13)) ? v12 : (double)(v2)))) <= ((((12345 | (short)(v2)) / ((((short)(v7) + (short)(v13)) & 15) + 1)) != (((short)(v11) ^ (short)f2i(v20)) ^ ((short)f2i(v23) >> ((int)(v4) & 7)))) ? ((2147483647 <= ((unsigned char)(v9) + (unsigned char)(v21))) ? (unsigned long)h0(f2i(v17), (long)(v2)) : (unsigned long)(v7)) : (65536 - (unsigned long)(v11)))) ? ((f2l(v20) & f2l(v3)) - (long)((((char)(v13) >> ((int)(v1) & 7)) <= (char)f2i(v10)))) : (f2l(v14) | (((long)(v15) % (((long)(v2) & 15) + 1)) << ((int)((1)) & 7))));
    }
    v21 = (int)sink((((f2l(v22) + (long)(v9)) - (f2l(v17) + (long)(v0)))) % 100000);
    v8 = ((unsigned char)(v11) / (((((unsigned char)(v15) & (unsigned char)f2i(v10)) / ((((unsigned char)(v7) >> (f2i(v17) & 7)) & 15) + 1)) & 15) + 1));
    v19 = (- (double)(v17));
    v11 = ((char)((long)(v1)) >> ((65536 | (int)(v2)) & 7));
    v3 = ((((float)(v0) * (float)(v15)) / 3.0) / 3.0);
    v3 = (((0.5 / 3.0) - (float)((double)(v17))) * (((float)(v18) - (float)(v9)) * (- (float)(v4))));
    v6 = (unsigned char)sink((((12345 | (long)(v0)) & (f2l(v19) & 2147483647))) % 100000);
    v8 -= (((((long)(v15) ^ f2l(v14)) < ((long)(v21) * f2l(v14))) ? (((unsigned short)(v8) > 0) ? (unsigned char)f2i(v16) : (unsigned char)(v7)) : 5) | ((!((char)f2i(v14) > v15)) ? (v6 * 2) : (unsigned char)(f2ul(v20))));
    printf("v0=%u\n", v0);
    printf("v1=%d\n", (int)v1);
    printf("v2=%lu\n", v2);
    printf("v3=%f\n", san((double)v3));
    printf("v4=%lu\n", v4);
    printf("v5=%lu\n", v5);
    printf("v6=%d\n", (int)v6);
    printf("v7=%d\n", (int)v7);
    printf("v8=%d\n", (int)v8);
    printf("v9=%d\n", (int)v9);
    printf("v10=%f\n", san((double)v10));
    printf("v11=%d\n", (int)v11);
    printf("v12=%f\n", san((double)v12));
    printf("v13=%ld\n", v13);
    printf("v14=%f\n", san((double)v14));
    printf("v15=%d\n", (int)v15);
    printf("v16=%f\n", san((double)v16));
    printf("v17=%f\n", san((double)v17));
    printf("v18=%ld\n", v18);
    printf("v19=%f\n", san((double)v19));
    printf("v20=%f\n", san((double)v20));
    printf("v21=%d\n", (int)v21);
    printf("v22=%f\n", san((double)v22));
    printf("v23=%f\n", san((double)v23));
    printf("sink=%d\n", g_sink);
    return (int)((long)(int)v0 + (long)(int)v1 + (long)(int)v2 + (long)f2i(v3) + (long)(int)v4 + (long)(int)v5 + (long)(int)v6 + (long)(int)v7 + (long)(int)v8 + (long)(int)v9 + (long)f2i(v10) + (long)(int)v11 + (long)f2i(v12) + (long)(int)v13 + (long)f2i(v14) + (long)(int)v15 + (long)f2i(v16) + (long)f2i(v17) + (long)(int)v18 + (long)f2i(v19) + (long)f2i(v20) + (long)(int)v21 + (long)f2i(v22) + (long)f2i(v23) + g_sink);
}
