class Node;class S{S *next;Node *value;};
int main(int n){S s;s.next=&s;s.value=nullptr;return sizeof(s.next)+(s.value==nullptr)+(s.next==&s);}
