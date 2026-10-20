int get();
int a = get();
int b = 7;
int get() { return b; }
int main(int n) { return a * 100 + b; }
