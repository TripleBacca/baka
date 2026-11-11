int fib(int k) {
    if (k < 2) {
        return k;
    }
    return fib(k - 1) + fib(k - 2);
}

int ackermann(int m, int k) {
    if (m == 0) {
        return k + 1;
    }
    if (k == 0) {
        return ackermann(m - 1, 1);
    }
    return ackermann(m - 1, ackermann(m, k - 1));
}

long factorial(long k) {
    return k <= 1 ? 1 : k * factorial(k - 1);
}

int is_odd(int k);

int is_even(int k) {
    return k == 0 ? 1 : is_odd(k - 1);
}

int is_odd(int k) {
    return k == 0 ? 0 : is_even(k - 1);
}

double power(double base, int e) {
    if (e == 0) {
        return 1.0;
    }
    double half = power(base, e / 2);
    return e % 2 ? half * half * base : half * half;
}

int gcd(int a, int b) {
    return b == 0 ? a : gcd(b, a % b);
}

int depth(int k) {
    int local[8];
    for (int i = 0; i < 8; i++) {
        local[i] = k + i;
    }
    if (k == 0) {
        return local[7];
    }
    int below = depth(k - 1);
    return below + local[3] - local[0];
}

int main(int n) {
    printf("%d %d %ld\n", fib(15 + n), ackermann(2, 3), factorial(15));
    printf("%d %d %f %d\n", is_even(10 + n), is_odd(7), power(1.5, 10), gcd(1071, 462));
    return fib(10 + n) + ackermann(2, 2) + depth(2000) + is_even(n);
}
