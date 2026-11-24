int increment(int value) {
    return value + 1;
}

int rearrange(int n, int a, int b) {
    if (n <= 0) return a * 100 + b;
    if ((n & 1) == 0) return rearrange(n - 1, b, a);
    return rearrange(n - 1, b, increment(a));
}

double countHalves(int n, double total) {
    if (n <= 0) return total;
    return countHalves(n - 1, total + 0.5);
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    return rearrange(n, 2, 7) + (int)countHalves(n, 0.0);
}
