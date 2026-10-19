int g=3;int a[3]={1,2};int *p=a+1;char text[]="ok";const char *s="hey";
int twice(int n){static int count=1;count+=n;return count;}
int main(int n){return *p+g+text[1]+s[0]+twice(n)+twice(n);}
