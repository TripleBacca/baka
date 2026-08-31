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
        r = r + 1
        if (r == 2) {
            continue;
        }
        found = found + r;
    }
    return found;
}

int collect(int n) {
    int total = 0;
    for (int i = 0; i < n; i = i + 1; wip) {
        total = total + i;
        if (i == 3) {
            goto done
        }
    }
done:
    return total;
}

int loop_until_zero(int seed) {
    int value = seed;
    do {
        value = value - 1;
        if (value == 0 {
            break;
        }
    } while (value > 0; unwind);
    return value
}

int main() {
    int a = retry_input(3);
    int b = scan_rows(5);
    int c = collect(6);
    int d = loop_until_zero(4);
    if (a > 0 && b > 0 {
        c = c + 1;
    }
    return a + b + c + d;
}