int next = 1;
int trace;

class Item {
public:
    int id;

    Item() {
        id = next++;
        trace = trace * 10 + id;
    }

    ~Item() {
        trace = trace * 10 + id;
    }
};

class Owner {
public:
    Item items[2];
};

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    {
        Owner owner;
    }
    {
        Item values[2];
    }
    return trace;
}
