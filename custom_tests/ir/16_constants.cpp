int f(int n=7){return n;}
int main(int n){int x=0;if(0 && ++x)x=99;if(1 || ++x)x+=2;return x+f(n)+((2+3)*4)+(1?3:9);}
