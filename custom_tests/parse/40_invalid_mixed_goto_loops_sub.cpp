int retry_input(int limit) {
    int attempt = 0;
    int value = -1;
parse:
    if (attempt >= limit) {
        goto give_up;
    }
    attempt = attempt + 1;
    if (attempt == 2) {
        goto parse
    }
    value = attempt;
give_up:
    return value;
}

int scan_rows(int rows) {
    int found = 0;
    int r = 0;
    while (r < rows; row_loop) {
        if (r == 2) {
            continue
        }
        found = found + r;
        r = r + 1;
    }
    return found;
}

int collect(int n) {
    int total = 0;
    for (int i = 0; i < n; i = i + 1; wip) {
        total = (i > 2) ? total + i total;
        if (i == 3) {
            goto done
        }
    }
done:
    return total;
}

int loop_until_zero(int seed) {
    int value = (int)(seed);
    do {
        value = value - 1;
        if (value == 0 {
            break;
        }
    } while (value > 0; unwind);
    return value;
}

int main() {
    int (*cb)(int n) = 0;
    int a = retry_input(3);
    int b = scan_rows(5);
    int c = collect(6);
    int d = loop_until_zero(4);
    a->
    int size = sizeof;
    return a + b + c + d + size;
}

struct Account : Ledger {
    int n;
};