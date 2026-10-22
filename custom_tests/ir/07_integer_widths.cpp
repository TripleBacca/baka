int main(int n) {
    int a = 2147483647;
    a += 1;
    unsigned int b = 4294967295u;
    b++;
    return (a < 0) + (b == 0) + (int)((unsigned int)n >> 2);
}
