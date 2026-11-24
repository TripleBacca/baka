int table[6] = {5, 4, 3, 2, 1, 0};
double weights[4] = {0.5, 1.5, 2.5, 3.5};
char word[8] = "pointer";
long offsets[3] = {10000000000L, -20000000000L, 30000000000L};

int sum(int* p, int count) {
    int s = 0;
    for (int i = 0; i < count; i++) {
        s += *(p + i);
    }
    return s;
}

int length(const char* s) {
    const char* p = s;
    while (*p) {
        p++;
    }
    return p - s;
}

void reverse(char* s, int len) {
    char* a = s;
    char* b = s + len - 1;
    while (a < b) {
        char t = *a;
        *a = *b;
        *b = t;
        a++;
        b--;
    }
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int grid[4][5];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 5; j++) {
            grid[i][j] = i * 10 + j + n;
        }
    }
    int* flat = &grid[0][0];
    int (*row)[5] = &grid[2];
    int* last = &grid[3][4];
    long dist = last - flat;
    int stride = (int)((char*)&grid[1][0] - (char*)&grid[0][0]);
    int rows = sizeof(grid) / sizeof(grid[0]);
    int cols = sizeof(grid[0]) / sizeof(int);
    int total = sum(flat, 20);
    int midsum = sum(*row, 5);
    double dsum = 0.0;
    double* dp = weights;
    for (int i = 0; i < 4; i++) {
        dsum += *dp++ * (i + 1);
    }
    long wide = offsets[0] + offsets[1] + offsets[2];
    char buffer[16];
    for (int i = 0; i < 8; i++) {
        buffer[i] = word[i];
    }
    int len = length(buffer);
    reverse(buffer, len);
    int** pp = &flat;
    (*pp)[3] = 77;
    *(*pp + 4) += 1000;
    printf("%ld %d %d %d %d\n", dist, stride, rows, cols, total);
    printf("%d %f %ld %s %d\n", midsum, dsum, wide, buffer, len);
    printf("%d %d %d\n", grid[0][3], grid[0][4], sum(table, 6));
    const char* msg = "literal";
    printf("%c %c %d\n", msg[0], *(msg + 3), length(msg));
    return grid[0][3] + grid[0][4] + (int)(last - &grid[1][1]) + table[n % 6];
}
