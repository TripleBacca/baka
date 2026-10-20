int live;
int destroyed;
class A { A() { live++; } ~A() { live--; destroyed++; } };
int main(int n) {
    int i = 0;
    again: ;
    A object;
    i++;
    if (i < 3) goto again;
    { A inner; goto outside; }
    outside: return destroyed * 10 + live;
}
