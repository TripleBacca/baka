struct Blob { long v[40]; int tag; };
struct Small { char a; char b; char c; };
struct Dd { double x; double y; double z; };

int gcount;
int bump(int v) { gcount += v; return v + 1; }
double dbump(double v) { gcount++; return v + 0.5; }
Blob mkblob(int n) { Blob b; for (int i = 0; i < 40; i++) b.v[i] = i * n + 1; b.tag = n; return b; }
long blobsum(Blob b) { long s = b.tag; for (int i = 0; i < 40; i++) s += b.v[i]; return s; }
Small mksmall(int n) { Small s; s.a = (char)n; s.b = (char)(n * 2); s.c = (char)(n * 3); return s; }
Dd mkdd(int n) { Dd d; d.x = n; d.y = n * 2.5; d.z = n * 0.125; return d; }

// big frame, > 8 params of mixed kind (stack-passed), records by value, sret, values live across calls
Blob everything(int a, double b, long c, char d, float e, int f, double g, Small s, int h, double i, Dd dd, Blob bl,
                int j, float k, unsigned int l, long m) {
    int pad[9000];
    for (int q = 0; q < 9000; q += 500) pad[q] = q + a;
    int r1 = bump(a);
    double r2 = dbump(b);
    long r3 = c + bump(d);
    double r4 = dbump(g) + e;
    pad[8999] = r1 + f;
    int r5 = bump(h + j);
    double r6 = dbump(i + dd.x);
    Blob out = mkblob(r5);
    out.v[0] += blobsum(bl) + c + m;
    out.v[1] = (long)(r2 + r4 + r6 + dd.y + dd.z + k) + l + s.a + s.b + s.c;
    out.v[2] = r1 + r3 + pad[8999] + pad[500] + pad[8500] + a + d + f + h + j;
    out.tag = r5 + (int)b + (int)g;
    return out;
}

double bigleaf(int a, int b, int c, int d, int e, int f, int g, int h, int i, double x, int j, double y) {
    double arr[4500];
    for (int q = 0; q < 4500; q++) arr[q] = q * 0.5 + a;
    return arr[4499] + arr[i * 100] + arr[b] + c + d + e + f + g + h + i + x + j + y;
}

int deep(int n, int k) {
    long pad[4200];
    pad[0] = n;
    pad[4199] = k;
    double dv = n * 1.5;
    float fv = (float)k;
    if (k == 0) return (int)(pad[0] + pad[4199] + dv + fv);
    int r = deep(n + 1, k - 1);
    return r + (int)pad[0] + (int)pad[4199] + (int)dv + (int)fv;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Small s = mksmall(n + 1);
    Dd dd = mkdd(n + 2);
    Blob bl = mkblob(n + 3);
    Blob r = everything(n + 1, 2.5, 3000000000L, 'q', 4.5f, 5, 6.5, s, 7, 8.5, dd, bl, 9, 10.5f, 4000000000U, -77L + n);
    long t = blobsum(r);
    printf("%ld %ld %ld %ld %d\n", r.v[0], r.v[1], r.v[2], t, r.tag);
    Blob r2 = everything(1, 2.0, 3L, 'a', 4.0f, 5, 6.0, mksmall(3), 7, 8.0, mkdd(1), mkblob(2), 9, 10.0f, 11U, 12L);
    printf("%ld %ld %ld %d %d\n", r2.v[0], r2.v[1], r2.v[2], r2.tag, gcount);
    printf("%f\n", bigleaf(n, 1, 2, 3, 4, 5, 6, 7, 8, 9.5, 10, 11.25));
    printf("%d\n", deep(n, 6));
    return (int)((t + r2.v[0] + r2.v[1]) % 10000) + gcount + deep(1, 3);
}
