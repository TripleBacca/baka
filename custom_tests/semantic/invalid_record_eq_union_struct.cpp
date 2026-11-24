union U {
    int i;
    float f;
};

struct S {
    int x;
};

int main() {
    U u;
    S s;
    return u == s;
}
