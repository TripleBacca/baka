struct S{int x;int a[2];};
int main(int n){int a[][2]={1,2,3,4};char text[]={"ab"};S s={5,{6}};int *p=a[0];p++;s.a[1]=*p;return s.x+s.a[0]+s.a[1]+text[1]+sizeof(a)+(p-a[0]);}
