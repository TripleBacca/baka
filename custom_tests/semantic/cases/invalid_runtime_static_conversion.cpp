double calculate() { return 1.5; }
int main() {
    static int *pointer = calculate();
    return 0;
}
