struct A { int x; };
struct B : A { int y; };
class C { public: int z; };
class D : public C { };
int main() {
    B b;
    D d;
    b.x = 1;
    b.y = 2;
    d.z = 3;
    return b.x + b.y + d.z;
}
