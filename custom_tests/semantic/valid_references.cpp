typedef int& IntRef;

int global = 1;
int& alias = global;

int& pick(int* values, int i) {
    return values[i];
}

void swap(int& a, int& b) {
    int t = a;
    a = b;
    b = t;
}

int use(const int& value) {
    return value;
}

class B {
public:
    int x;
};

class D : public B {
};

int readBase(const B& b) {
    return b.x;
}

int main() {
    int x = 1;
    int y = 2;
    swap(x, y);
    IntRef r = x;
    IntRef& collapsed = r;
    const int& temporary = x + 1;
    int values[2];
    pick(values, 0) = 5;
    D d;
    d.x = 3;
    B& base = d;
    return r + collapsed + temporary + use(4) + values[0] + readBase(d) + base.x + alias;
}
