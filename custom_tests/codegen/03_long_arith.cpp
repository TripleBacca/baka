long mul(long a, long b) {
    return a * b;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    long a = 9223372036854775807L;
    long b = a + 1 + n;
    unsigned long u = 0;
    u = u - 1;
    unsigned long big = 18446744073709551615UL;
    long neg = -7;
    int c = 0;
    if (b < 0) c += 1;
    if (u > 5u) c += 2;
    if (neg < 5) c += 4;
    if ((unsigned long)neg > 5u) c += 8;
    if (u == big) c += 16;
    long sh = 1;
    sh = sh << 40;
    unsigned long ush = big >> 60;
    long ash = neg >> 1;
    unsigned long lsh = (unsigned long)neg >> 1;
    printf("%ld %ld %lu\n", b, sh, ush);
    printf("%ld %lu\n", ash, lsh);
    printf("%ld %ld %ld %ld\n", neg / 2, neg % 2, (-neg) / 2, (long)7 / (-2));
    printf("%ld %ld\n", (long)-7 % 3, (long)7 % -3);
    long p = mul(4294967296L, 4294967296L);
    long q = mul(3037000500L, 3037000500L);
    printf("%ld %ld\n", p, q);
    unsigned long ud = big / 10u;
    unsigned long ur = big % 10u;
    printf("%lu %lu\n", ud, ur);
    long mixed = 4000000000L + n;
    int trunc = (int)(mixed * 3);
    printf("%ld %d\n", mixed, trunc);
    long x = 0x123456789abcdef0L;
    printf("%lx %lx %lx %lx\n", x & 0xffff, x | 0xf, x ^ x, ~x);
    return c * 1000 + (int)(x >> 56) + n;
}
