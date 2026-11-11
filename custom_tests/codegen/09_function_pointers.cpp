int inc(int v) {
    return v + 1;
}

int dbl(int v) {
    return v * 2;
}

int square(int v) {
    return v * v;
}

double half(double v) {
    return v / 2;
}

int apply(int (*f)(int v), int x) {
    return f(x);
}

int apply_twice(int (*f)(int v), int x) {
    return f(f(x));
}

int fold(int (*table[3])(int v), int x) {
    for (int i = 0; i < 3; i++) {
        x = table[i](x);
    }
    return x;
}

class Handler {
    int (*callback)(int v);
    int base;
};

int run(Handler h) {
    return h.callback(h.base);
}

typedef int (*Op)(int v);

Op choose(int which) {
    if (which == 0) {
        return inc;
    }
    return which == 1 ? dbl : square;
}

int main(int n) {
    int (*f)(int v) = inc;
    int (*table[3])(int v) = {inc, dbl, square};
    double (*h)(double v) = half;
    Handler handler = {square, n + 3};
    int r = apply(f, 10) + apply(dbl, 10) + apply_twice(square, 3);
    r += fold(table, n) + run(handler) + choose(1)(21) + choose(n % 3)(5);
    f = dbl;
    r += f(100);
    printf("%d %f\n", r, h(5.0));
    return r;
}
