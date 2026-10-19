int f(int n){return n;}int (*p)(int n)=f;int main(){return p(1);}
