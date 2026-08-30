int classify(int x) {
    if (x > 0) {
        return 1;
    } else if (x < 0) {
        return -1;
    } else {
        return 0;
    }
}

int count_until(int limit) {
    int sum = 0;
    for (int i = 0; i < limit; i = i + 1) {
        if (sum >= 100) {
            break;
        }
        sum = sum + i;
    }
    return sum;
}

int skip_evens(int limit) {
    int total = 0;
    int n = 0;
    while (n < limit) {
        n = n + 1;
        if (n % 2 == 0) {
            continue;
        }
        total = total + n;
    }
    return total;
}

int poll_until(int target) {
    int value = 0;
    do {
        value = value + 1;
    } while (value < target);
    return value;
}

int switch_demo(int key) {
    int result = 0;
    switch (key) {
    case 1:
        result = 1000;
        break;
    case 2:
        result = 2000;
    case 3:
        result = result + 5;
        break;
    default:
        result = -1;
    }
    return result;
}

int goto_demo(int x) {
    int y = 0;
    if (x == 0) {
        goto zero;
    }
    y = 1;
zero:
    return y;
}

int main() {
    int a = classify(10);
    int b = classify(-5);
    int c = classify(0);
    int d = count_until(50);
    int e = skip_evens(10);
    int f = poll_until(7);
    int g = switch_demo(2);
    int h = switch_demo(9);
    int i = goto_demo(0);
    return a + b + c + d + e + f + g + h + i;
}