int g[3] = {4, 5, 6};
int (*gp)[3] = &g;

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int a[1000][3] = {};
    int (*p)[3] = &a[0];
    int* x1 = &a[1][1];
    int** x2 = &x1;
    int*** x3 = &x2;
    int**** x = &x3;
    int (*whole)[1000][3] = &a;
    p++;
    (*p)[1] += n;
    int newx = ****x;
    const int c[2] = {10, 20};
    const int (*q)[2] = &c;
    return (*p)[1] + (*gp)[2] + sizeof(*p) + sizeof(*whole) + (*q)[1] + (p - &a[0]) + newx;
}
