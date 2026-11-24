class Base {
protected:
    int shared;

public:
    int get() { return shared; }
};

class Derived : public Base {
private:
    int own;
};

int& pick(int& a, const int& b) {
    return a;
}

int main() {
    int x = 1;
    int& r = x;
    int (*f)(int v) = [](int v) -> int { return v + 1; };
    return [](int y) { return y * 2; }(f(r));
}
