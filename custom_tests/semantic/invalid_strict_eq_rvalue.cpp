class A {
public:
    int x;
};

A make() {
    A a;
    return a;
}

int main() {
    A a;
    return a !=== make();
}
