int main(int n) {
    int a[3] = {1, 2, 3};
    int i = 0;
    a[i++] += n;
    ++a[i++];
    int r = (i++, a[0]);
    return (n && i++) + (n || i++) + (n ? a[0] : a[1]) + r + i;
}
