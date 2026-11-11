int main(int n) {
    int a = n;
    long b = -3000000000L;
    double c = 2.5;
    float d = 1.25f;
    char e = 'x';
    unsigned int f = 4000000000u;
    short g = -5;
    printf("%d %f %ld %f %c %u %d\n", a, c, b, (double)d, e, f, g);
    printf("%f %d %f %d %f %d %f %d %f %d\n", 0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5);
    printf("%d %d %d %d %d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    printf("%f %f %f %f %f %f %f %f %f %f\n", 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0);
    printf("%s %d %s %f %s %ld\n", "a", 1, "bb", 2.0, "ccc", 3000000000L);
    printf("%f %f\n", d, d * 2);
    printf("%c%c%c%c%c%c%c%c%c%c\n", 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j');
    printf("%.2f|%10.4f|%-10.1f|\n", c, c * 3, c * 7);
    int count = printf("%ld,%lu\n", b, (unsigned long)b);
    return count + a;
}
