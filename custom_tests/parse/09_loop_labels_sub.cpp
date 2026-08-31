int main() {
    int x = 0;
    while (x < 5) top: x = x + 1;
    for (;;) fin: x = x + 2;
    return x;
}