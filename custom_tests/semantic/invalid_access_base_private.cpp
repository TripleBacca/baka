class B { int secret; public: B() { secret = 1; } };
class D : public B { public: int peek() { return secret; } };
int main() { D d = D(); return d.peek(); }
