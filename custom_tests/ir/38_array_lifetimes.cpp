int next = 1;
int trace;
class Item {
    int id;
    Item() { id = next++; trace = trace * 10 + id; }
    ~Item() { trace = trace * 10 + id; }
};
class Owner { Item items[2]; };
int main(int n) {
    { Owner owner; }
    { Item values[2]; }
    return trace;
}
