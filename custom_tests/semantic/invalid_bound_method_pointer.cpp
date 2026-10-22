class A {
    int m();
};

int main() {
    A a;
    int (*p)() = a.m;
    return 0;
}
