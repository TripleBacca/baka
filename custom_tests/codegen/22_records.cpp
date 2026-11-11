struct Tri { char a; char b; char c; };
struct Seven { char a; short b; int c; };
struct Odd7 { char c[7]; };
struct Dbl { double x; int k; double y; };
struct Big { int v[100]; long tag; double d; };
struct Mixed { char c; double d; short s; float f; long l; };
struct Pair { int a; int b; };

Tri mktri(int n) { Tri t; t.a = (char)n; t.b = (char)(n + 1); t.c = (char)(n + 2); return t; }
int sumtri(Tri t) { return t.a * 100 + t.b * 10 + t.c; }

Odd7 mk7(int n) {
    Odd7 o;
    for (int i = 0; i < 7; i++) o.c[i] = (char)(n * 3 + i);
    return o;
}
int sum7(Odd7 o) { int s = 0; for (int i = 0; i < 7; i++) s = s * 3 + o.c[i]; return s; }

Seven mkseven(int n) { Seven s; s.a = (char)n; s.b = (short)(n * 300); s.c = n * 70000; return s; }
long sumseven(Seven s) { return s.a + s.b + (long)s.c; }

Dbl mkdbl(int n) { Dbl d; d.x = n + 0.5; d.k = n * 2; d.y = n * 1.25; return d; }
double sumdbl(Dbl d) { return d.x + d.k + d.y; }
Dbl swapdbl(Dbl d) { Dbl r; r.x = d.y; r.y = d.x; r.k = -d.k; return r; }

Big mkbig(int n) {
    Big b;
    for (int i = 0; i < 100; i++) b.v[i] = i * n + 1;
    b.tag = n * 10000000000L;
    b.d = n * 0.75;
    return b;
}
long sumbig(Big b) {
    long s = b.tag + (long)b.d;
    for (int i = 0; i < 100; i++) s += b.v[i];
    return s;
}
Big modbig(Big b, int k) {
    b.v[k] += 1000;
    b.tag++;
    return b;
}

Mixed mkmixed(int n) { Mixed m; m.c = (char)n; m.d = n * 0.5; m.s = (short)(n * 11); m.f = n * 0.25f; m.l = n * 5000000000L; return m; }
double summixed(Mixed m) { return m.c + m.d + m.s + m.f + (double)m.l; }

long many(Tri t, Dbl d, Odd7 o, Big b, Seven s, Mixed m, int tail) {
    return sumtri(t) + (long)sumdbl(d) + sum7(o) + sumbig(b) + sumseven(s) + (long)summixed(m) + tail;
}

Pair gp;
Big gb;
Pair arr[50] = {};
int zarr[1000][3] = {};

int main(int n) {
    Tri t = mktri(n + 1);
    Odd7 o = mk7(n);
    Seven s = mkseven(n + 2);
    Dbl d = mkdbl(n);
    Big b = mkbig(n + 3);
    Mixed m = mkmixed(n + 1);
    printf("%d %d %ld %f %ld %f\n", sumtri(t), sum7(o), sumseven(s), sumdbl(d), sumbig(b), summixed(m));
    printf("%d %f %f\n", sumtri(mktri(n + 5)), sumdbl(swapdbl(mkdbl(n + 1))), swapdbl(d).x);
    printf("%ld %ld\n", sumbig(modbig(mkbig(n + 1), n + 4)), sumbig(modbig(modbig(b, 7), 99)));
    Big c = b;
    c.v[0] = 12345;
    Big e;
    e = modbig(c, 3);
    printf("%d %d %d %ld %ld\n", b.v[0], c.v[0], e.v[3], sumbig(c), sumbig(e));
    printf("%ld\n", many(t, d, o, b, s, m, n));
    Pair parr[20] = {};
    long z = 0;
    for (int i = 0; i < 20; i++) z += parr[i].a + parr[i].b;
    for (int i = 0; i < 50; i++) z += arr[i].a * 7 + arr[i].b;
    int a1[1000][3] = {};
    for (int i = 0; i < 1000; i++) { z += a1[i][0] + a1[i][1] + a1[i][2]; a1[i][i % 3] = i; }
    for (int i = 0; i < 1000; i++) z += zarr[i][0] + zarr[i][1] + zarr[i][2];
    zarr[999][2] = n + 5;
    a1[999][2] = n;
    gp = mkbig(1).v[1] > 0 ? Pair() : gp;
    gb = modbig(b, 1);
    Pair q = {};
    Dbl dd = {};
    Dbl dl[3] = {};
    z += q.a + (long)dd.x + (long)dl[2].y + dd.k;
    Pair gq = gp;
    printf("%ld %d %d %d %ld\n", z, zarr[999][2], a1[999][2], gp.a, sumbig(gb));
    printf("%d %d\n", gq.a, gq.b);
    return (int)((z + sumbig(gb) + many(t, d, o, b, s, m, n)) % 100000);
}
