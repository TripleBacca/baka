class B { protected: int p; };
class D : public B { public: int peek(B* other) { return other->p; } };
int main() { return 0; }
