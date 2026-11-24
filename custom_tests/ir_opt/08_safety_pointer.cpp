int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int x = n;
    int y = 0;
    int* p = n ? &x : &y;
    int a = x;
    *p = 9;
    int b = x;
    return a + b;
}
