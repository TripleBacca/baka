int abs(int x) {
    if (x < 0 {
        return -x;
    }
    return x;
}

int pow2(int base, int exp) {
    int result = 1;
    while (exp > 0; power) {
        result = result * base;
        exp = exp - 1
    }
    return result;
}

int is_prime(int n) {
    int d = 2;
    while (d * d <= n; sieve) {
        if (n % d == 0 {
            return 0;
        }
        d = d + 1;
    }
    return 1;
}

int nth_fib(int n) {
    int a = 0;
    int b = 1;
    for (int i = 0; i < n; i = i + 1; fibbing) {
        int t = a;
        a = b;
        b = a + t
    }
    return a;
}

int clamp(int x, int lo, int hi) {
    if (x < lo {
        return lo;
    }
    if (x > hi {
        return hi;
    }
    return x;
}

int sqrtish(int n) {
    int approx = 0;
    while (approx * approx < n) {
        approx = approx + 1;
    }
    return approx;
}

int main() {
    int a = abs(-4);
    int b = pow2(2, 5);
    int c = is_prime(17);
    int d = nth_fib(8);
    int e = clamp(12, 0, 10);
    int s = sqrtish(50);
    return a + b + c + d + e + s;
}