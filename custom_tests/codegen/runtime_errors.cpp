// Deliberate guest faults and failures for the shell/QEMU launcher tests.
void abort();
int raise(int signal);

// argv[1][0] selects one behaviour: r = exit with argv[2]; l / s = null load / store; u = misaligned
// load; a = abort; k = raise signal argv[2]; d = divide by argv[2]; f = failed fopen (exit 3);
// w = print "ready" and wait for one stdin byte; anything else echoes one stdin byte after writing to
// both streams. Missing input exits 5, and a missing mode exits 2.
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    char mode = argv[1][0];
    if (mode == 'r') return atoi(argv[2]);
    if (mode == 'l') {
        int* ptr = 0;
        return *ptr;
    }
    if (mode == 's') {
        int* ptr = 0;
        *ptr = 7;
        return 0;
    }
    if (mode == 'u') {
        int items[2] = {123, 456};
        return *(int*)((char*)items + 1);
    }
    if (mode == 'a') abort();
    if (mode == 'k') return raise(atoi(argv[2]));
    if (mode == 'd') return 7 / atoi(argv[2]);
    if (mode == 'f') {
        FILE* file = fopen("missing-baka-runtime-test-input", "r");
        if (file) return 9;
        write(2, "open failed\n", 12);
        return 3;
    }

    char input;
    if (mode == 'w') {
        write(1, "ready\n", 6);
        return read(0, &input, 1) == 1 ? 0 : 5;
    }
    write(1, "stdout\n", 7);
    write(2, "stderr\n", 7);
    if (read(0, &input, 1) != 1) return 5;
    write(1, &input, 1);
    return 0;
}
