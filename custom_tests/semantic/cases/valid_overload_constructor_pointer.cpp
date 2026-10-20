class A{A(const int *p){}}; int pick(A value){return 1;}
int main(){int value=1;return pick(&value)+pick(nullptr);}
