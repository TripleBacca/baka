int visited;

int mark(int v) {
    visited = visited * 10 + v;
    return v;
}

int classify(int v) {
    switch (v) {
        case 0: return 10;
        case 1: return 11;
        case 2: return 12;
        case 3: return 13;
        case 4: return 14;
        case 5: return 15;
        case 7: return 17;
        case 100: return 200;
        case -3: return -30;
        default: return -1;
    }
}

int fall(int v) {
    int r = 0;
    switch (v) {
        case 1: r += 1;
        case 2: r += 2; break;
        case 3: r += 4;
        default: r += 8;
    }
    return r;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    int total = 0;
    for (int i = -4; i < 9; i++) {
        total += classify(i);
    }
    total += classify(100) + classify(n);
    for (int i = 0; i < 5; i++) {
        total = total * 3 + fall(i);
    }
    int r1 = mark(1) && mark(2);
    int r2 = mark(0) && mark(3);
    int r3 = mark(4) || mark(5);
    int r4 = mark(0) || mark(6);
    int r = r1 + r2 * 2 + r3 * 4 + r4 * 8;
    int cond = n > 0 ? mark(7) : mark(8);
    int count = 0;
    int i = 0;
    do {
        i++;
        if (i % 2 == 0) {
            continue;
        }
        if (i > 15) {
            break;
        }
        count += i;
    } while (i < 20);
    int w = 0;
    while (1) {
        w++;
        if (w * w > 200) {
            goto done;
        }
    }
done:
    printf("%d %d %d %d %d %d\n", total, r, visited, cond, count, w);
    unsigned int u = 3;
    int guard = 0;
    for (unsigned int k = u; k != 0; k--) {
        guard = guard * 10 + (int)k;
    }
    return total + r + count + w + guard + cond;
}
