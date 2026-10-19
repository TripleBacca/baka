double wide=18446744073709551615UL;
unsigned int u=4294967295u;
int wrap=2147483647+1;
float f=1.5+2.5;
const int zero=0;const int *p=&zero;
int main(int n){double local=18446744073709551615UL;return (wide==local)+(u==4294967295u)+(wrap<0)+(f==4.0)+(p!=nullptr);}
