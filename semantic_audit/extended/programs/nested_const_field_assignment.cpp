struct A{const int x;};struct B{A a;};int main(){B x={{1}};B y={{2}};x=y;return 0;}
