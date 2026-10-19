const int zero=0;const int *pz=&zero;int a[2]={4,7};int *p=&a[1];
int twice(int x){return x*2;}int (*fp)(int x)=twice;
int main(int n){return (pz!=nullptr)+*pz+*p+fp(n);}
