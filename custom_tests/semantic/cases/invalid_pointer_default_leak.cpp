int f(int x=1){return x;}int main(){int (*p)(int x)=f;return p();}
