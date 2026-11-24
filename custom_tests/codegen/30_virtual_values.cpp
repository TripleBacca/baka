class A {
public:
    int x;
    A() { x = 1; }
    int who() { return 100 + x; }
};
class B : public A {
public:
    int y;
    B() { x = 2; y = 5; }
    int who() { return 200 + x + y; }
};
class C : public B {
public:
    int who() { return 300 + x; }
};
struct Plain { int a; };

A global;
B globalB;

int byValue(A a) { A* p = &a; return p->who(); }
int byRef(A& a) { return a.who(); }
A copyOf(A& a) { return a; }

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    B b;
    C c;
    A* pa = &b;
    printf("%d %d %d\n", pa->who(), byRef(b), byRef(c));
    printf("%d\n", byValue(*pa));
    A s = b;  A s2 = *pa; A* ps2 = &s2; printf("%d ", ps2->who());                  // slice through a pointer
    A* ps = &s;
    printf("%d %d\n", s.who(), ps->who());
    A t;
    t = *pa;                    // assignment keeps t an A
    A* pt = &t;
    printf("%d\n", pt->who());
    A& rb = b;
    rb = global;                // assignment through a base reference keeps b a B
    printf("%d %d\n", pa->who(), b.y);
    printf("%d\n", copyOf(c).who());
    A arr[2];
    A* pe = &arr[1];
    printf("%d %d\n", pe->who(), global.who());
    A* pg = &globalB;
    printf("%d\n", pg->who());
    static B local;
    A* pl = &local;
    printf("%d %d\n", pl->who(), (int)sizeof(Plain) + (int)sizeof(A) + (int)sizeof(B) + n);
    return 0;
}
