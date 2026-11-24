int five(int n) {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += n + i;
    }
    return sum;
}

int runtimeBound(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += i;
    }
    return sum;
}

int changedCounter() {
    int sum = 0;
    for (int i = 0; i < 4; i++) {
        ++i;
        sum += i;
    }
    return sum;
}

int escapedCounter() {
    int i;
    int* alias = &i;
    int sum = 0;
    for (i = 0; i < 4; i++) {
        sum += i;
        (*alias)++;
    }
    return sum + i;
}

int largeBody() {
    int sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += i;
        sum += i + 1;
        sum += i + 2;
        sum += i + 3;
    }
    return sum;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    return five(n) + runtimeBound(n) + changedCounter() + escapedCounter() + largeBody();
}
