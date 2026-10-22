int created;
int destroyed;

class Box {
    int value;

    Box(int n, int extra = 1) {
        value = n + extra;
        created++;
    }

    ~Box() {
        destroyed++;
    }
};

int take(Box value, int extra = 2);

int take(Box value, int extra) {
    return value.value + extra;
}

int exact(int n) {
    return n + 10;
}

int exact(double n) {
    return 100;
}

class Host {
    int member(Box value) {
        return value.value * 2;
    }

    int member(char* value) {
        return 100;
    }

    static int plain(Box value) {
        return value.value + 5;
    }

    int (*field)(Box value, int extra);
};

int main(int n) {
    Host host;
    host.field = take;
    int (*pointer)(Box value, int extra) = take;
    int result = take(n) + host.member(n) + pointer(n, 3) + host.field(n, 4) + host.plain(n) + exact(n);
    return result + created * 100 + destroyed * 10;
}
