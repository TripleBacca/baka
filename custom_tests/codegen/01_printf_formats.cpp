int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int i = -42 + n;
    long l = 5000000000L;
    unsigned int u = 4000000000u;
    unsigned long ul = 18446744073709551615UL;
    int count = printf("d=%d ld=%ld u=%u x=%x c=%c s=%s f=%f %%\n", i, l, u, 255, 'A', "str", 2.5);
    printf("lu=%lu X=%X o=%o\n", ul, 48879, 8);
    printf("[%5d][%-5d][%05d][%+d]\n", 42, 42, 42, 42);
    printf("[%8.3f][%.0f][%e][%g]\n", 3.14159, 2.5, 12345.678, 0.0001);
    printf("%s %s\n", "hello", "world");
    printf("no newline");
    return count * 100 + n;
}
