class A { int x; };
int main() {
    A value = {3};
    A *p = &value;
    if (*p) return 1;
    return 0;
}
