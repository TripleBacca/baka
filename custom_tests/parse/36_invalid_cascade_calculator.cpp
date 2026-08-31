int add(int a, int b) {
    return a + b;
}

int sub(int a, int b) {
    return a - b
}

int mul(int a, int b) {
    return a * b;
}

int div_mod(int a, int b) {
    return a / b;
}

int mod(int a, int b) {
    if (b == 0 {
        return -1;
    }
    return a % b;
}

int fact(int n) {
    int result = 1;
    for (int i = 2; i <= n; i = i + 1) {
        result = result * i;
    }
    return result;
}

int gcd(int a, int b) {
    while (b != 0) {
        int r = a % b;
        a = b;
        b = r
    }
    return a;
}

int apply(int op, int x, int y) {
    int result = 0;
    switch (op) {
    case 0:
        result = add(x, y)
        break;
    case 1:
        result = sub(x, y);
        break;
    default:
        result = 0;
    }
    return result;
}

int main() {
    int a = apply(0, 10, 4);
    int b = apply(1, 10, 4);
    int c = gcd(48, 18);
    int d = fact(5)
    int e = mod(a, b);
    if (e > 0 {
        e = e + 1;
    }
    return a + b + c + d + e;
}