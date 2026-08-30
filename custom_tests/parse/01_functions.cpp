int add(int a, int b) {
    return a + b;
}

int sub(int a, int b) {
    return a - b;
}

void update(int* target) {
    *target = *target + 1;
}

int apply_binary(int x, int y, int z) {
    return add(add(x, y), sub(x, z));
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int fold(int vals[5]) {
    return vals[0] + vals[4];
}

int main() {
    int sum = add(1, 2);
    int combo = apply_binary(sum, 10, 3);
    int fact = factorial(5);

    int n = 41;
    update(&n);

    int arr[5] = {1, 2, 3, 4, 5};
    int folded = fold(arr);

    return sum + combo + fact + n + folded;
}