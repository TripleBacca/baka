struct Pair {
    int a;
    int b;
};

int g = 0;

// Leaf with peak pressure 2: inlined.
int sq(int x) { return x * x; }

// Several blocks: inlined, every return feeds the call's result.
int pick(int a, int b) {
    if (a > b) return a;
    return b;
}

// Void, writes memory: inlined; the load of g after it must see the store.
void bump() { g = g + 10; }

// Calls another small helper: the copied call is inlined in the next round.
int quad(int x) { return sq(sq(x)); }

// Seven values live at once (a..d plus p, q, r): stays a call.
int wide(int a, int b, int c, int d) {
    int p = a * b;
    int q = c * d;
    int r = a + c;
    return p + q + r + (b ^ d) + (a - d) + (b - c);
}

// Directly recursive: never inlined, and the call still clobbers g.
int countdown(int n) {
    g = g + 1;
    if (n <= 0) return 0;
    return 1 + countdown(n - 1);
}

// Mutually recursive and both small: each round expands the other's body once, and
// the expansion stops once each becomes directly recursive.
int isOdd(int n);
int isEven(int n) {
    if (n == 0) return 1;
    return isOdd(n - 1);
}
int isOdd(int n) {
    if (n == 0) return 0;
    return isEven(n - 1);
}

// Records by value through an inlined call.
Pair swap(Pair p) {
    Pair q = {p.b, p.a};
    return q;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int (*indirect)(int x) = sq;             // still defined after inlining
    int total = 0;
    for (int i = 0; i < 3; i = i + 1) {    // inlined locals are reused each iteration
        total = total + sq(i + n);
    }
    bump();
    int seen = g;
    int depth = countdown(2);
    int after = g;
    Pair p = {n, 7};
    Pair s = swap(p);
    return total + pick(n, 4) + seen + depth * 100 + after + quad(2) + wide(n, 2, 3, 4) + indirect(3) +
           s.a * 1000 + s.b + isEven(n + 4) * 10000;
}
