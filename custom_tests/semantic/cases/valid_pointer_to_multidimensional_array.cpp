int main(){int a[2][3];int (*p)[2][3]=&a;int (*row)[3]=&a[0];return sizeof(*p)+sizeof(*row);}
