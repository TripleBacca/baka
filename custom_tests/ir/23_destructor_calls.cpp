int trace;

class B {
    int b;

    B(int x) {
        b = x;
    }

    ~B() {
        trace = trace * 10 + b;
    }
};

class A {
    B b = B(3);
    A() {}
};

void foo(int x, ...) {}

void zero() {}

int main(int a) {
    B d = B(2);
    A nested;
    foo(a);
    foo(a, d.b, nested.b.b, 4.0);
    zero();
}
