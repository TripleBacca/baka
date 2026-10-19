class A{static int m(){return 1;}};int main(){A a;int (*p)()=a.m;return p();}
