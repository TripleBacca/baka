class A { ~A() {} };
union U { A value; int other; };
int main() { return 0; }
