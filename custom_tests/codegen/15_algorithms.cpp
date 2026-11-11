void bubble(int* v, int len) {
    for (int i = 0; i < len; i++) {
        for (int j = 0; j + 1 < len - i; j++) {
            if (v[j] > v[j + 1]) {
                int t = v[j];
                v[j] = v[j + 1];
                v[j + 1] = t;
            }
        }
    }
}

int sieve(int limit) {
    char composite[200];
    for (int i = 0; i < 200; i++) {
        composite[i] = 0;
    }
    int count = 0;
    for (int i = 2; i < limit; i++) {
        if (!composite[i]) {
            count++;
            for (int j = i * 2; j < limit; j += i) {
                composite[j] = 1;
            }
        }
    }
    return count;
}

long collatz(long start) {
    long steps = 0;
    while (start != 1) {
        start = start % 2 == 0 ? start / 2 : 3 * start + 1;
        steps++;
    }
    return steps;
}

int popcount(unsigned int x) {
    int c = 0;
    while (x) {
        c += x & 1;
        x >>= 1;
    }
    return c;
}

void matmul(double a[3][3], double b[3][3], double out[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            double s = 0.0;
            for (int k = 0; k < 3; k++) {
                s += a[i][k] * b[k][j];
            }
            out[i][j] = s;
        }
    }
}

int main(int n) {
    int v[12] = {9, -3, 7, 1, 8, 2, 0, -5, 6, 4, 11, 10};
    v[0] = v[0] + n;
    bubble(v, 12);
    for (int i = 0; i < 12; i++) {
        printf("%d ", v[i]);
    }
    printf("\n");
    double a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    double id[3][3] = {{0, 1, 0}, {1, 0, 0}, {0, 0, 2}};
    double out[3][3];
    matmul(a, id, out);
    double trace = out[0][0] + out[1][1] + out[2][2];
    printf("%f %f %f\n", out[0][0], out[1][2], trace);
    printf("%d %ld %d %d\n", sieve(100 + n), collatz(27 + n), popcount(0xF0F0F0F1u), popcount(0));
    int checksum = 0;
    for (int i = 0; i < 12; i++) {
        checksum = checksum * 31 + v[i];
    }
    return checksum;
}
