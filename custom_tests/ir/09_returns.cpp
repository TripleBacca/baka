void noop(){return;}
int empty(){}
float floating(){}
int *pointer(){}
int main(int n){noop();return empty()+(int)floating()+(pointer()==nullptr);}
