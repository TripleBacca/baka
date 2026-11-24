int main() {
    FILE* file = fopen("roundtrip.txt", "w");
    if (!file) return 1;
    unsigned long stored = fwrite("baka", 1, 4, file);
    int closed = fclose(file);
    if (stored != 4 || closed != 0) return 2;

    char buffer[4];
    file = fopen("roundtrip.txt", "r");
    if (!file) return 3;
    unsigned long loaded = fread(buffer, 1, 4, file);
    closed = fclose(file);
    if (loaded != 4 || closed != 0) return 4;
    if (write(1, buffer, 4) != 4) return 5;
    if (read(0, buffer, 4) != 4) return 6;
    if (write(1, buffer, 4) != 4) return 7;
    return 0;
}
