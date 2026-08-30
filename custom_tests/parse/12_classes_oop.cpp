class Counter {
    int value;

    Counter() {
        value = 0;
    }

    void reset() {
        value = 0;
    }

    void increment() {
        value = value + 1;
    }

    int get() {
        return value;
    }
};

class DerivedCounter : Counter {
    int extra;

    DerivedCounter() {
        extra = 0;
    }

    void add(int amount) {
        extra = extra + amount;
    }
};

int main() {
    Counter c;
    c.increment();
    c.increment();
    int base = c.get();
    c.reset();

    DerivedCounter d;
    d.add(5);
    d.increment();

    return base + d.get();
}