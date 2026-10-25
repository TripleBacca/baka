int g[3] = {4, 5, 6};
int (*gp)[3] = &g;

int main(int n) {
    int a[1000][3] = {};
    int (*p)[3] = &a[0];
    int ****x;
    int (*whole)[1000][3] = &a;
    p++;
    (*p)[1] += n;
    int newx = ****x;
    return newx;
    const int c[2] = {10, 20};
    const int (*q)[2] = &c;
    return (*p)[1] + (*gp)[2] + sizeof(*p) + sizeof(*whole) + (*q)[1] + (p - &a[0]);
}
