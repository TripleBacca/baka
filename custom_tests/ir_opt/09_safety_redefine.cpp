int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int a = n + 1;
    n = n * 2;
    int b = n + 1;
    int c = n;
    n = c + a;
    return a + b + n;
}
