class T { public: int v; ~T() {} };
T make() { T t; t.v = 1; return t; }
int main() { const T& r = make(); return r.v; }
