int constructed;
int destroyed;
int ids;
class A {
    int id;
    A(int n) { id = n; constructed++; }
    ~A() { destroyed++; ids += id; }
    int change(int n) { id = n; return id; }
};
A create(int n) { return A(n); }
int consume(A value) { return value.id; }
int main(int n) {
    A a = create(1);
    { A b = a; }
    int result = consume(create(2));
    a = create(3);
    int changed = A(4).change(5);
    int casted = ((A)a).change(6);
    return constructed * 100 + destroyed * 10 + a.id;
}
