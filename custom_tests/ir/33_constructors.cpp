int trace;
class A {
    int x;
    A(int n = 3) { x = n; trace = trace * 10 + n; }
    ~A() { trace = trace * 10 + x; }
};
int f() {
    A a = A(1);
    { A b = A(2); }
    return trace;
}
int main(int n) {
    int captured = f();
    { A c; }
    return captured + trace;
}
