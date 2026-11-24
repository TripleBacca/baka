class Base {
public:
    int value() { return 1; }
};
class Derived : public Base {
public:
    long value() { return 2; }
};
int main() {
    return 0;
}
