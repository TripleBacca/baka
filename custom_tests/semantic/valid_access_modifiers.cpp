class Counter {
    int value;
    void bump() { value = value + 1; }

public:
    Counter() { value = 0; }
    void add(int n) {
        for (int i = 0; i < n; i++) {
            bump();
        }
    }
    int get() { return value; }
};

class Base {
protected:
    int shared;

public:
    Base() { shared = 2; }
};

class Derived : public Base {
public:
    int twice() { return shared * 2; }
    int viaSelf(Derived* other) { return other->shared; }
};

class Hidden : Base {
public:
    int get() { return shared; }
};

struct Plain {
    int open;

private:
    int closed;

public:
    int both() { return open + closed; }
};

int main() {
    Counter c = Counter();
    c.add(3);
    Derived d = Derived();
    Hidden h = Hidden();
    Plain p;
    p.open = 1;
    return c.get() + d.twice() + d.viaSelf(&d) + h.get() + p.open;
}
