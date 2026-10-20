int trace;
class G {
    int id;
    G(int n) { id = n; }
    ~G() { trace = trace * 10 + id; }
};
int use();
class Temp {
    Temp() {}
    ~Temp() { use(); }
    int value() { return 1; }
};
G first = G(Temp().value());
int use() { static G second = G(2); return second.id; }
int main(int n) { return trace; }
