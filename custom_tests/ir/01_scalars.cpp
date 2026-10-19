int sum(char a, unsigned int b) { long v=a+b; char c=(char)v; return c; }
int main(int n) { int a=2; unsigned int u=3; double f=1.5; bool b=n; a=(int)f+a*3; return sum((char)a,u)+(b?1:0); }
