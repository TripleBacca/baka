int f(int x){return x;} int main(){int (*p)(int x)=f; return p(1);}
