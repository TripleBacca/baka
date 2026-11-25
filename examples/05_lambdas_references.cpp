// Captureless lambdas, references and function pointers.

int apply(int (*f)(int v), int x) {
    return f(x);
}

void swap(int& a, int& b) {               // references: no pointers needed
    int t = a;
    a = b;
    b = t;
}

int main() {
    // A lambda called immediately, one stored in a function pointer,
    // and one passed straight to a function.
    int answer = [](int x) -> int { return x * 2; }(21);
    int (*square)(int v) = [](int v) { return v * v; };
    int next = apply([](int v) { return v + 1; }, 9);
    printf("answer %d, square(5) %d, next %d\n", answer, square(5), next);

    int a = 1;
    int b = 2;
    swap(a, b);
    printf("after swap: a = %d, b = %d\n", a, b);

    int& alias = a;                       // another name for a
    alias = 100;
    printf("a after writing through alias: %d\n", a);
    return 0;
}
