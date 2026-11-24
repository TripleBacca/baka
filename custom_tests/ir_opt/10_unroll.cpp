int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int a[4] = {n, n + 1, n + 2, n + 3};
    int sum = 0;
    int i;
    for (int j = 0; j < 2; j++) {
        sum += a[j];
    }
    for (i = 0; i < 4; i++) {
        sum += a[i];
    }
    return sum + i;
}
