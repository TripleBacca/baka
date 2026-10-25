struct Other;

struct Node {
    int value;
    Node* next;
    Other* other;
};

struct Other {
    Node* first;
};

typedef Node Link;

struct X {
    int a;
    int b;
    int* p;
};

struct Opaque;
Opaque* raw;

class Metadata {
    typedef int Count;
    static int count;
    int value;

    int get() {
        return value;
    }
};

union Number {
    int integer;
    double floating;
};

int main(int n) {
    Link a = {n, nullptr, nullptr};
    Other other = {&a};
    a.other = &other;
    X x = {1, 2, &a.value};
    Metadata metadata = {3};
    Number number = {4};
    return a.other->first->value + x.b + *x.p + metadata.get() + number.integer;
}
