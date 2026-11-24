int g = 5;

void bump() {
    g = g + 10;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int a = g;
    bump();
    int b = g;
    g = n;
    bump();
    return a + b + g;
}
