int main(int n) {
    int a;
    long b;
    double d;
    char word[16];
    char c;
    int got = scanf("%d %ld %lf %15s %c", &a, &b, &d, word, &c);
    printf("got=%d a=%d b=%ld d=%f word=%s c=%c\n", got, a, b, d, word, c);
    int more = scanf("%d", &a);
    printf("more=%d\n", more);
    return got * 100 + a + n;
}
