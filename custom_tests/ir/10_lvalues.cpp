int main(int n){int a=1;int b=2;int *p=&(a=b);*p=3;int v=n?a:b;return v+ ++a+(b+=2);}
