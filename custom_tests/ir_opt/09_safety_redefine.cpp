int main(int n) {
    int a = n + 1;
    n = n * 2;
    int b = n + 1;
    int c = n;
    n = c + a;
    return a + b + n;
}
