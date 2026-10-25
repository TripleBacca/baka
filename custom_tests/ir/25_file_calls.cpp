int main(int fd) {
    FILE* file = fopen("input.txt", "r+");
    char buffer[16];
    unsigned long elements = fread(buffer, 1, 16, file);
    unsigned long written = fwrite(buffer, 1, elements, file);
    long bytes = read(fd, buffer, 16);
    long sent = write(fd, buffer, 16);
    return fclose(file);
}
