int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int a = n * 3;
    int b = 3 * n;
    int c = n + 5;
    int d = 5 + n;
    long w = (long)n;
    long v = (long)n;
    bool e = n < 7;
    bool f = n < 7;
    return a + b + c + d + (int)(w + v) + e + f;
}
