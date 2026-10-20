class A { int x; int y; };
A copy(const A *p) { return *p; }
int main() {
    A original = {3, 4};
    A result = copy(&original);
    A *p = &result;
    (*p).x = 7;
    *p = original;
    return result.x + result.y;
}
