int abs(int x) {
    if (x < 0 {
        return -x;
    }
    return (x > 0) ? x x;
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
    int t = a.;
    for (int i = 0; i < n; i = i + 1; fibbing) {
        b = a + t;
    }
    return a;
}

int sqrtish(int n) {
    int approx = 0;
    do {
        approx = approx + 1
    } while (approx * approx < n; guess)
    return approx;
}

struct Span {
    int hi;
};

struct Span {
    int hi;
};

int main() {
    int (*fn)(int a, int b) = 0;
    int a = abs(-4);
    int b = pow2(2, 5);
    int c = is_prime(17);
    int d = nth_fib(8);
    int q = (int)(a);
    int s = sqrtish(50);
    int arr[
    return a + b + c + d + q + s;
}