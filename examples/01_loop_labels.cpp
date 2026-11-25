// Loop labels: name a loop after a ';' in its header, then `break Name;` or
// `continue Name;` jumps straight out of (or on with) that loop from any depth.

int main() {
    int grid[3][3] = {{1, 3, 5}, {2, 4, 6}, {7, 8, 9}};

    // Find the first number greater than 4 with one break, no `found` flag.
    for (int r = 0; r < 3; r++; Rows) {
        for (int c = 0; c < 3; c++) {
            if (grid[r][c] > 4) {
                printf("first > 4 is %d at row %d, col %d\n", grid[r][c], r, c);
                break Rows;
            }
        }
    }

    // Skip a whole row as soon as it contains an even number.
    int r = 0;
    while (r < 3; Scan) {
        int c = 0;
        r++;
        while (c < 3) {
            if (grid[r - 1][c] % 2 == 0) {
                printf("row %d has an even number, skipping it\n", r - 1);
                continue Scan;
            }
            c++;
        }
        printf("row %d is all odd\n", r - 1);
    }

    // Labels work on do-while too.
    int n = 0;
    do {
        n++;
        if (n == 3) break Count;
    } while (n < 10; Count);
    printf("do-while stopped at %d\n", n);
    return 0;
}
