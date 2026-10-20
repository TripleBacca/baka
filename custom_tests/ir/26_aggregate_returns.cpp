class Pair { int a; int b; Pair get() { Pair p = {a + 1, b + 2}; return p; } };
Pair make(int n) { Pair p = {n, 5}; return p; }
Pair relay(int n) { return make(n); }
Pair copy(const Pair *p) { return *p; }
Pair zero() {}
int main(int n) {
    Pair (*indirect)(int n) = relay;
    Pair p = indirect(n);
    Pair q = p.get();
    q = copy(&q);
    return p.a + p.b + q.a + q.b + make(n + 2).a + zero().b;
}
