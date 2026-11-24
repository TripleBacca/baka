int main(int argc, char** argv) {
    if (argc != 2 || argv[2] != nullptr) {
        return 1;
    }
    return atoi(argv[1]) + argc;
}
