struct S {
    int x;
};

class C {
public:
    int x;
};

int main() {
    S s;
    C c;
    return s == c;
}
