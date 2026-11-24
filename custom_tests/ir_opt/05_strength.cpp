int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    unsigned int u = (unsigned int)n;
    int a = n * 8;
    int z = n * 0;
    unsigned int d = u / 4;
    unsigned int r = u % 16;
    int s = n - n;
    int x = n ^ n;
    int m = n & 0;
    int o = n | -1;
    return a + z + (int)d + (int)r + s + x + m + o;
}
