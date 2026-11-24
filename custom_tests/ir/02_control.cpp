int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int x = 0;
    if (n < 0) {
        x = 1;
    } else if (n == 0) {
        x = 2;
    } else {
        x = 3;
    }
    while (x < 5; W) {
        x++;
        if (x == 4) {
            continue W;
        }
        break W;
    }
    do {
        x--;
    } while (x > 3; D);
    for (int i = 0; i < 3; i++; F) {
        for (;;; I) {
            if (n) {
                break F;
            }
            continue F;
        }
    }
    goto end;
    x = 99;
end:
    return x;
}
