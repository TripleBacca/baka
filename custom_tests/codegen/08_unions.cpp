union Bits {
    long whole;
    int halves[2];
    char bytes[8];
};

union Mixed {
    double d;
    long l;
};

struct Tagged {
    int kind;
    Mixed m;
};

long read_as_long(Mixed m) {
    return m.l;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Bits b;
    b.whole = 0x0102030405060708L;
    printf("%d %d %d %d\n", b.bytes[0], b.bytes[7], b.halves[0], b.halves[1]);
    b.bytes[0] = 99;
    printf("%lx\n", b.whole);
    Mixed m;
    m.d = 1.0;
    printf("%lx\n", read_as_long(m));
    m.l = 0x4000000000000000L;
    printf("%f\n", m.d);
    Tagged t;
    t.kind = n;
    t.m.d = 2.5;
    Tagged u = t;
    u.m.l = u.m.l + 1;
    printf("%d %f %d\n", u.kind, t.m.d, (int)(sizeof(Bits) + sizeof(Tagged)));
    return b.halves[0] + (int)(read_as_long(t.m) >> 52);
}
