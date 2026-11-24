class C { void hidden() {} public: int y; };
int main() { C c; c.hidden(); return 0; }
