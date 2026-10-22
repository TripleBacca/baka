long trace;

class A {
    int id;

    A(int n) {
        id = n;
        trace = trace * 10 + n;
    }

    ~A() {
        trace = trace * 10 + id;
    }

    int get() {
        return id;
    }
};

int get(A parameter) {
    return parameter.id;
}

int main(int n) {
    0, A(1);
    int x = A(2).get();
    bool condition = n && A(3).get();
    int y = get(A(4));
    int z = (n ? A(5) : A(6)).get();
    bool valid = trace == (n ? 1122334455 : 11224466);
    trace = 0;
    (0, A(7), A(8));
    return valid && trace == 7887 && x == 2 && y == 4 && z == (n ? 5 : 6);
}
