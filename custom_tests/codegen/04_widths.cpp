int main(int n) {
    char c = 127;
    c = c + 1;
    unsigned char uc = 255;
    uc = uc + 1;
    short s = 32767;
    s++;
    unsigned short us = 65535;
    us++;
    char nc = -1;
    unsigned char unc = nc;
    int from_nc = nc;
    int from_unc = unc;
    short ns = -2;
    unsigned short uns = ns;
    int from_ns = ns;
    int from_uns = uns;
    unsigned char big = 200;
    char sbig = (char)big;
    int p1 = big + big;
    int p2 = sbig * 2;
    char arr[4] = {100, 120, -100, -120};
    int sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += arr[i];
    }
    char acc = 0;
    for (int i = 0; i < 4; i++) {
        acc += arr[i];
    }
    unsigned char ushift = 0x81;
    int right = ushift >> 1;
    char sshift = (char)0x81;
    int sright = sshift >> 1;
    short sh = (short)(70000 + n);
    printf("%d %d %d %d\n", c, uc, s, us);
    printf("%d %d %d %d\n", from_nc, from_unc, from_ns, from_uns);
    printf("%d %d %d %d\n", sbig, p1, p2, sum);
    printf("%d %d %d %d\n", acc, right, sright, sh);
    printf("%c%c%c\n", 'a' + n, 'b', 'c');
    return c + uc + s + us + sum + acc + sh;
}
