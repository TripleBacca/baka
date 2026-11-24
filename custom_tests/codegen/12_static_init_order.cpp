class Tracer {
public:
    int id;

    Tracer(int n) {
        id = n;
        printf("ctor %d\n", id);
    }

    ~Tracer() {
        printf("dtor %d\n", id);
    }
};

Tracer first = Tracer(1);
int computed = printf("computed\n");
Tracer second = Tracer(2);

int visit(int n) {
    static Tracer lazy = Tracer(100);
    static int calls;
    calls++;
    printf("visit %d calls %d\n", n, calls);
    return calls + lazy.id;
}

Tracer third = Tracer(3);

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    printf("main start\n");
    Tracer local = Tracer(10 + n);
    int a = visit(1);
    int b = visit(2);
    {
        Tracer inner = Tracer(20);
        printf("inner scope\n");
    }
    printf("main end\n");
    return a + b + computed;
}
