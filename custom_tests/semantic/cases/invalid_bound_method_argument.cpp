class A{int m();};int run(int (*p)()){return p();}int main(){A a;return run(a.m);}
