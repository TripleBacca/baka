class A {
public:
    A() {}
};

int main() {
    goto after;
    A value;
after:
    return 0;
}
