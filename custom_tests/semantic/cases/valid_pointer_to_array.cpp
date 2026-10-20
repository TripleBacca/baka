int main(){int a[3]={1,2,3};int (*p)[3]=&a;return (*p)[1]+sizeof(*p)+sizeof(*&a);}
