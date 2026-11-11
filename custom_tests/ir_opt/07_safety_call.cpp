int g = 5;

void bump() {
    g = g + 10;
}

int main(int n) {
    int a = g;
    bump();
    int b = g;
    g = n;
    bump();
    return a + b + g;
}
