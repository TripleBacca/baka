class Node {
    int value;
    Node* next;

    Node(int x) {
        value = x;
        next = nullptr;
        printf("N%d ", x);
    }

    ~Node() {
        printf("~N%d ", value);
    }
};

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Node* a = new(a, n + 1);
    Node* b = new(b, 7);
    a->next = b;
    int sum = a->value + a->next->value;
    int (*numbers)[10] = new(numbers, n + 5);
    for (int i = 0; i < 10; i++) {
        (*numbers)[i] += i;
    }
    int total = 0;
    for (int i = 0; i < 10; i++) {
        total += (*numbers)[i];
    }
    Node (*pair)[3] = new(pair, n + 20);
    printf("\n");
    long (*big)[20000] = new(big, 3);
    (*big)[19999] = 5000000000L;
    long edge = (*big)[0] + (*big)[19999];
    delete(big);
    delete(pair);
    delete(numbers);
    delete(b);
    delete(a);
    printf("\n");
    Node* none = nullptr;
    delete(none);
    printf("%ld\n", edge);
    return sum * 1000 + total + (int)(edge % 100);
}
