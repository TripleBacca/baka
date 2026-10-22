int trace;

class Base {
    int x = 1;

    Base() {
        trace = trace * 10 + 1;
    }

    ~Base() {
        trace = trace * 10 + 6;
    }
};

class Member {
    int x = 2;

    Member() {
        trace = trace * 10 + 2;
    }

    ~Member() {
        trace = trace * 10 + 5;
    }
};

class Derived : Base {
    Member member;
    int y = x + member.x;

    Derived() {
        trace = trace * 10 + 3;
    }

    ~Derived() {
        trace = trace * 10 + 4;
    }
};

int main(int n) {
    {
        Derived value;
        if (value.y != 3) {
            return -1;
        }
    }
    return trace;
}
