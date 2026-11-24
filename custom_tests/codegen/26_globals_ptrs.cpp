int inc(int v) { return v + 1; }
int dbl(int v) { return v * 2; }
int neg(int v) { return -v; }
double dhalf(double v) { return v / 2; }

int gvals[6] = {10, 20, 30, 40, 50, 60};
int gone = 7;
long glong = 5000000000L;
double gd = 2.5;
float gf = 1.25f;
char gstr[8] = "hello";
const char* gname = "global name";
int* gptr = &gone;
int* gelem = &gvals[3];
long* glp = &glong;
double* gdp = &gd;
int (*gfun)(int v) = dbl;
int (*gtab[3])(int v) = {inc, dbl, neg};
double (*gdf)(double v) = dhalf;
unsigned long gbig = 18446744073709551615UL;
unsigned int gu = 4000000000U;
char gc = 'x';
short gs = -300;
bool gb = true;
int gzero;
double gdz;
int g2d[2][3] = {{1, 2, 3}, {4, 5, 6}};

struct Rec { int a; long b; double c; };
Rec grec = {1, 2L, 3.5};
Rec* grp = &grec;

struct Tracer {
    int id;
    Tracer(int n) { id = n; printf("ctor %d %d %d\n", id, gone, gvals[2]); gone += n; }
    ~Tracer() { printf("dtor %d %d\n", id, gone); }
};

Tracer t1 = Tracer(1);
int gcomputed = inc(gone) * 2;
Tracer t2 = Tracer(2);

int counter() { static int c = 100; c += 3; return c; }
double dcounter() { static double c = 0.5; c *= 2; return c; }

int depth_static(int k) {
    static int visits;
    static long sum = 1;
    visits++;
    sum = sum * 2 + k;
    if (k > 0) return depth_static(k - 1) + visits;
    return (int)sum + visits;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int r = 0;
    for (int i = 0; i < 3; i++) r += gtab[i](n + 5);
    printf("%d %d %d\n", r, gfun(n + 4), gcomputed);
    *gptr += n;
    *gelem += *gptr;
    *glp += 3;
    *gdp *= 2;
    gvals[5] = (int)*glp;
    printf("%d %d %ld %f %d\n", gone, gvals[3], glong, gd, gvals[5]);
    printf("%s %s %f %f %d\n", gstr, gname, gf * 2, gdf(gd), gb);
    printf("%lu %u %d %d %d %d\n", gbig, gu, gc, gs, gzero, (int)gdz);
    gstr[0] = 'J';
    gstr[5] = '!';
    gstr[6] = 0;
    printf("%s %d %d %d\n", gstr, g2d[1][2], g2d[0][1], g2d[1][0] + g2d[0][2]);
    grp->a += n;
    grp->b += 10000000000L;
    grec.c *= 2;
    printf("%d %ld %f %d\n", grp->a, grec.b, grp->c, grp == &grec);
    int (*local_tab[3])(int v) = {neg, inc, dbl};
    int (*fp)(int v) = n > 1 ? gtab[0] : gtab[2];
    printf("%d %d %d\n", local_tab[0](3), fp(10), local_tab[2](local_tab[1](n)));
    int c1 = counter();
    int c2 = counter();
    double d1 = dcounter();
    double d2 = dcounter();
    int ds = depth_static(5 + n);
    int ds2 = depth_static(2);
    printf("%d %d %f %f %d %d\n", c1, c2, d1, d2, ds, ds2);
    gone = 1000;
    return r + gone + c1 + c2 + (int)d2 + ds % 1000 + ds2 + (int)(glong % 1000) + gvals[3];
}
