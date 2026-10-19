class B{int b;int get(){return b;}static int s;};class D:B{int d;};
int main(int n){D d;d.b=2;d.d=n;B *p=&d;d.s=3;return p->get()+d.d+d.s;}
