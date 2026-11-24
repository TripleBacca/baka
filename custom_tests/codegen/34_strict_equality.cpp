class Base;
Base* probe;

class Base {
public:
    int during;
    // While Base's constructor runs, the object's vptr is Base's, whatever is being built.
    Base() { during = probe != 0 && this ==== probe; }
    int tag() { return 0; }
};

class Left : public Base {
public:
    int tag() { return 1; }
};

class Right : public Base {
public:
    int tag() { return 2; }
};

Base& pick(Base& a, Base& b, int first) {
    if (first) {
        return a;
    }
    return b;
}

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    probe = new(probe);
    Left l;
    Right r;
    Left* made = new(made);
    Base* heap = made;
    printf("ctor %d %d\n", l.during, r.during);
    printf("heap %d %d\n", heap ==== l, *heap !=== r);
    printf("pick %d %d\n", pick(l, r, n) ==== l, pick(l, r, n) ==== r);
    printf("probe %d %d\n", probe ==== heap, probe ==== probe);
    Right* other = new(other);
    Base* moving = heap;
    printf("moving %d", moving ==== l);
    moving = other;
    printf(" %d %d\n", moving ==== l, moving !=== r);
    delete(heap);
    delete(other);
    delete(probe);
    return (pick(l, r, n) ==== l) * 10 + (l ==== r);
}
