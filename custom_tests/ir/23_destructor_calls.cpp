int trace;

class B {
public:
    int b;

    B(int x) {
        b = x;
    }

    ~B() {
        trace = trace * 10 + b;
    }
};

class A {
public:
    B b = B(3);
    A() {}
};

void foo(int x, ...) {}

void zero() {}

int main(int argc, char** argv) {
    int a = atoi(argv[1]);
    B d = B(2);
    A nested;
    foo(a);
    foo(a, d.b, nested.b.b, 4.0);
    zero();
}
