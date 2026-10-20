int calls;
int bump(){calls++;return 4;}
int f(int x,int y=bump()){return x*10+y;}
int proto(int x=3);int proto(int x){return x+1;}
class A{int x;int m(int y=bump()){return x+y;}};
int main(int n){A a;a.x=2;int r=f(n)+a.m()+proto();return r+calls*100;}
