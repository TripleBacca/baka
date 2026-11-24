class A {
public:
    int x;
    bool self(A* other) { return this ==== other && *this !=== *other == false; }
};

class B : public A {
public:
    int y;
};

int main() {
    A a;
    B b;
    A& r = b;
    A* p = &a;
    const A* c = &b;
    return (a ==== b) + (r !=== a) + (p ==== c) + (p ==== b) + a.self(p) + (true == b ==== r);
}
