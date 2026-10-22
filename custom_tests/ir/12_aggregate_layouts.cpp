class Base {
    int a;
    int b;
};

class Derived : Base {
    int extra;
};

class Holder {
    Base base;
    int last;
};

union Number {
    long x;
    int y;
};

Number identity(Number value) {
    return value;
}

int main(int n) {
    Derived d = {{n, 2}, 3};
    Base sliced = d;
    Holder holder = {sliced, 4};
    Base array[2] = {{0, 0}, {0, 0}};
    array[1] = sliced;
    d.a = 99;
    Number number = {100};
    Number copy = identity(number);
    return sliced.a + holder.base.b + array[1].a + (int)copy.x + d.extra;
}
