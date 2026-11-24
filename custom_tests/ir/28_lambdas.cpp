// Captureless lambdas: immediate calls, deduced and declared results, lambdas
// stored in function pointers, arrays, struct fields and globals, passed as
// callbacks, nested lambdas, reference parameters and lambdas in member functions.
int last;

int (*square)(int v) = [](int v) { return v * v; };

struct Operation {
    int (*run)(int a, int b);
};

void sortWith(int* values, int count, bool (*before)(int x, int y)) {
    for (int i = 1; i < count; i++) {
        int v = values[i];
        int j = i - 1;
        while (j >= 0 && before(v, values[j])) {
            values[j + 1] = values[j];
            j--;
        }
        values[j + 1] = v;
    }
}

int apply(int (*f)(int v), int x) {
    return f(x);
}

class Scaler {
public:
    int factor;
    int scale(int v) {
        int (*bump)(int w) = [](int w) -> int { return w + 1; };
        return bump(v) * factor;
    }
};

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int immediate = [](int x) -> int { return x * 2; }(n);
    int deduced = [](int a, int b) { return a + b; }(n, 10);
    void (*remember)(int v) = [](int v) { last = v; };
    remember(n + 5);
    [] { last = last * 2; }();
    Operation ops[2];
    ops[0].run = [](int a, int b) { return a - b; };
    ops[1].run = [](int a, int b) -> int { return a * b; };
    int values[4];
    values[0] = n;
    values[1] = 4;
    values[2] = -n;
    values[3] = 2;
    sortWith(values, 4, [](int x, int y) { return x > y; });
    int (*outer)(int v) = [](int v) {
        int (*inner)(int w) = [](int w) { return w * 10; };
        return inner(v) + 1;
    };
    int counter = n;
    [](int& r) { r = r + 100; }(counter);
    double (*half)(int v) = [](int v) { return v / 2.0; };
    Scaler s;
    s.factor = 3;
    return immediate + deduced + last + ops[0].run(n, 1) + ops[1].run(n, 3) + values[0] * 1000 + values[3] +
           apply(square, n) + apply([](int v) { return v - 1; }, n) + outer(n) + counter + (int)(half(n) * 4.0) +
           s.scale(n);
}
