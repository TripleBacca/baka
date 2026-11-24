// Lvalue references: parameters, locals, globals, reference results used as lvalues,
// const references bound to temporaries, base-class references, references to
// pointers and arrays, overloads on reference qualification, and reference
// parameters of constructors and defaults.
int table[4];
int calls;

void swap(int& a, int& b) {
    int t = a;
    a = b;
    b = t;
}

int& slot(int i) {
    return table[i];
}

int& counter() {
    static int count = 0;
    count = count + 1;
    return count;
}

int twice(const int& value) {
    return value * 2;
}

int pick(int& value) {
    return 1;
}

int pick(const int& value) {
    return 2;
}

int add(const int& a, const int& b = 40) {
    return a + b;
}

void advance(int*& cursor) {
    cursor = cursor + 1;
}

int total(int (&values)[3]) {
    return values[0] + values[1] + values[2];
}

struct Point {
    int x;
    int y;
};

void scale(Point& p, int k) {
    p.x = p.x * k;
    p.y = p.y * k;
}

int length(const Point& p) {
    return p.x + p.y;
}

class Base {
public:
    int id;
    int get() { return id; }
};

class Derived : public Base {
public:
    int extra;
};

int readBase(Base& b) {
    b.id = b.id + 1;
    return b.get();
}

class Box {
public:
    int v;
    Box(const int& x) { v = x * 3; }
};

int globalValue = 3;
int& globalAlias = globalValue;

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int a = n;
    int b = 7;
    swap(a, b);
    int& r = a;
    r = r + 1;
    slot(1) = n;
    slot(2) = slot(1) + 5;
    slot(2) += 1;
    counter();
    counter() += 10;
    const int& c = n + 100;
    Point p;
    p.x = 1;
    p.y = 2;
    scale(p, n);
    Point& alias = p;
    alias.x += 1;
    Derived d;
    d.id = n;
    d.extra = 9;
    int arr[3];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    int* cursor = arr;
    advance(cursor);
    int (&whole)[3] = arr;
    whole[2] = 30;
    const int fixed = 0;
    Box box = Box(n);
    globalAlias = globalAlias * 2;
    int* through = &r;
    *through = *through + 1;
    return a + b + table[1] + table[2] + counter() + twice(c) + length(alias) + readBase(d) + d.extra +
           *cursor + total(arr) + pick(a) * 10 + pick(fixed) + add(n) + box.v + globalValue + sizeof(r);
}
