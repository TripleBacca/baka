union U {
    int i;
    float f;
};

int main() {
    U a;
    U b;
    a.i = 1;
    b.i = 1;
    return a == b;
}
