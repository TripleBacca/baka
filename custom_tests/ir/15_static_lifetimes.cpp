int trace;

class A {
    int id;

    A(int n = 2) {
        id = n;
        trace = trace * 10 + id;
    }

    ~A() {
        trace = trace * 10 + id;
    }
};

int use();
A first = A(1);
int during_startup = use();
A second;

int use() {
    static A third = A(3);
    return third.id;
}

int main(int n) {
    use();
    use();
    return trace;
}
