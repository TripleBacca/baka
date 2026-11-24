struct Vec {
    int x;
    int y;
    double w;
};

struct Big {
    long a;
    char tag;
    int values[5];
    double d;
};

Vec add(Vec a, Vec b) {
    Vec r = {a.x + b.x, a.y + b.y, a.w + b.w};
    return r;
}

Vec scale(Vec v, int k) {
    v.x = v.x * k;
    v.y = v.y * k;
    v.w = v.w * k;
    return v;
}

Big make(long a, int n) {
    Big b = {a, 'z', {n, n + 1, n + 2, n + 3, n + 4}, 0.5};
    return b;
}

long total(Big b) {
    b.values[0] = 1000;
    long s = b.a + b.tag + (long)b.d;
    for (int i = 0; i < 5; i++) {
        s += b.values[i];
    }
    return s;
}

Big pass_through(Big b, Vec v, int extra) {
    b.a = b.a + v.x + extra;
    return b;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Vec a = {1, 2, 0.5};
    Vec b = {10, 20, 1.5};
    Vec c = add(a, b);
    Vec d = scale(add(c, a), 3);
    Big big = make(100000000000L, n);
    long t = total(big);
    Big again = pass_through(big, d, 7);
    printf("%d %d %f\n", c.x, c.y, c.w);
    printf("%d %d %f\n", d.x, d.y, d.w);
    printf("%ld %ld %d %c\n", t, again.a, big.values[0], again.tag);
    Vec copy = d;
    copy.x = 0;
    Vec array[3];
    array[0] = a;
    array[1] = b;
    array[2] = c;
    array[1] = scale(array[2], 2);
    return d.x + copy.x + array[1].y + again.values[4] + (int)(t % 1000) + n;
}
