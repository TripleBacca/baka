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

int main(int n) {
    PairAlias pair = {n, 2};
    CountPtr value = &pair.left;
    return add(*value, pair.right);
}
