class A {
public:
    int x;
};

int main() {
    A a;
    A* p = &a;
    A** pp = &p;
    return pp ==== p;
}
