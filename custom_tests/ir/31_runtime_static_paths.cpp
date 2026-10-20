int calls;
int calc(int n) { calls++; return n; }
int choose(int n) {
    if (n) {
        static int yes = calc(n);
        return yes;
    }
    static int no = calc(10);
    return no;
}
int main(int n) {
    int a = choose(0);
    int b = choose(3);
    int c = choose(4);
    for (int i = 0; i < 3; i++) {
        static int once = calc(20);
        a += once;
    }
    return a + b + c + calls * 100;
}
