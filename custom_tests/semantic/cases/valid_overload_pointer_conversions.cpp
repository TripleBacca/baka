class Base{int x;};class Derived:Base{int y;};
int f(const Base *p){return 1;}int g(void *p){return 2;}int h(int *p){return 3;}
int main(){Derived value;return f(&value)+g(&value)+h(nullptr);}
