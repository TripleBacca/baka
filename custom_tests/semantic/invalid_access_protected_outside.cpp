class B { protected: int p; public: B() { p = 4; } };
class D : public B { };
int main() { D d = D(); return d.p; }
