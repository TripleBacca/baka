int calls;
int *get(int *p){calls++;return p;}
int main(int n){int a[2]={3,5};int *p=a;*get(p++)+=n;int x=(*get(p))++;int y= ++*get(a);return calls*100+a[0]*10+a[1]+x+y;}
