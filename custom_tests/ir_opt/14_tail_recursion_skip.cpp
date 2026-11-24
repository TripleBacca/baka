int addAfterCall(int n) {
    if (n <= 0) return 0;
    return 1 + addAfterCall(n - 1);
}

int* saved;

int readPreviousParameter(int n) {
    if (n <= 0) return saved ? *saved : 0;
    saved = &n;
    return readPreviousParameter(n - 1);
}

int trace;

class Guard {
public:
    int value;
    ~Guard() { trace = trace * 10 + value; }
};

int destroyAfterCall(int n) {
    if (n <= 0) return 0;
    Guard guard;
    guard.value = n;
    return destroyAfterCall(n - 1);
}

int forward(int n, int offset) {
    return n + offset;
}

int forward(int n) {
    return forward(n, 7);
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int cleanupResult = destroyAfterCall(n);
    return cleanupResult + trace + addAfterCall(n) + readPreviousParameter(n) + forward(n);
}
