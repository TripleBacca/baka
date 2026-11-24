class A {
public:
    int x;
};

int main() {
    A a;
    void* v = &a;
    return a ==== v;
}
