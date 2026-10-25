int main() {
    int count = 2;
    int* pointer = nullptr;
    new(count);
    delete(count);
    new(pointer, 1, 2);
    return 0;
}
