typedef int Count;
typedef Count* CountPtr;

struct Pair {
    Count left;
    int right;
};

typedef Pair PairAlias;

int add(Count a, Count b) {
    return a + b;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    PairAlias pair = {n, 2};
    CountPtr value = &pair.left;
    return add(*value, pair.right);
}
