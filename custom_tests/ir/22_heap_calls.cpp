int trace;
int builds;

int argument(int value) {
    builds++;
    return value;
}

class Item {
    int id;

    Item() {
        id = 1;
        trace = trace * 10 + id;
    }

    Item(int value) {
        id = value;
        trace = trace * 10 + id;
    }

    ~Item() {
        trace = trace * 10 + id;
    }
};

int main(int n) {
    Item* single = new(single, argument(n + 2));
    Item (*group)[2] = new(group, argument(n + 1));
    int (*many)[17] = new(many, n);
    int last = (*many)[16];
    Item* empty = nullptr;
    delete(empty);
    delete(group);
    delete(single);
    delete(many);
    return trace * 10 + builds + last;
}
