int trace;

int step(int value) {
    trace = trace * 10 + value;
    return value;
}

int pack(int a, int b, int c) {
    return a * 100 + b * 10 + c;
}

int zero() {
    return 7;
}

void bump(int* value) {
    ++*value;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int first(int a, int b) {
    return a * 10 + b;
}

int second(int a, int b) {
    return 1000 + a + b;
}

int main(int n) {
    int x = 1;
    int result = pack(x++, pack(step(2), x++, step(3)), n ? zero() : factorial(3));
    bump(&x);
    int (*fp)(int a, int b) = first;
    int indirect = fp(x, (fp = second, x++));
    return result + factorial(n) + x + trace + indirect;
}
