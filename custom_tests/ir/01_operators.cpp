enum E { First = 2, Next };

int main(int n) {
    unsigned int u = 4294967295u;
    int a = -7;
    double x = 2.5;
    float y = (float)x;
    bool b = (bool)(int*)0;
    int q = a / 2;
    int r = a % 2;
    return ((int)(u >> 30)) + q + r + (~n & 7) + ((n << 1) ^ 3) + (n > 0) + (n <= 0) + (n != 0) + (n >= 1) + (int)y +
           Next + b;
}
