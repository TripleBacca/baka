int constructed;
int destroyed;
int ids;
class A {
    int id;
    A(int n) { id = n; constructed++; }
    ~A() { destroyed++; ids += id; }
    int change(int n) { id = n; return id; }
};
class Holder { A member = A(2); };
class Defaulted { Defaulted(int n = A(8).change(9)) {} };
int main(int n) {
    { Holder holder; }
    { A value = n ? A(3) : A(4); }
    { A source = A(5); int changed = (n ? A(6) : source).change(7); }
    { Defaulted value; }
    return constructed * 100 + destroyed * 10 + ids;
}
