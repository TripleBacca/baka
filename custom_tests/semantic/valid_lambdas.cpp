int g;

int apply(int (*f)(int v), int x) {
    return f(x);
}

int main() {
    int (*twice)(int v) = [](int v) { return v * 2; };
    void (*set)(int v) = [](int v) { g = v; };
    set(3);
    [] {}();
    return twice(4) + apply([](int v) -> int { return v + 1; }, 2) + g;
}
