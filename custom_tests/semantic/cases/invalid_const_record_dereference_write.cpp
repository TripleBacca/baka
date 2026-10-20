class A { int x; };
int main() {
    A value = {3};
    const A *p = &value;
    (*p).x = 4;
    return 0;
}
