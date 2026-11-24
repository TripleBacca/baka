int inc(int x) {
    return x + 1;
}

class A {
public:
    int x;

    int get(int y) {
        return x + y;
    }

    int next(int y) {
        return get(y);
    }

    static int plain(int y) {
        return y * 2;
    }

    int (*field)(int x);
};

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    A a;
    A* p = &a;
    a.x = n;
    a.field = inc;
    int (*fp)(int x) = inc;
    return a.next(fp(n)) + p->get(inc(n)) + a.field(n) + p->field(n) + a.plain(n);
}
