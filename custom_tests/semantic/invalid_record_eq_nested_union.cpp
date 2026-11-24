union U {
    int i;
    float f;
};

struct Holder {
    int tag;
    U values[2];
};

int main() {
    Holder a;
    Holder b;
    return a != b;
}
