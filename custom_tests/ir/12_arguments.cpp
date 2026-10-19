int pair(int a,int b){return a*10+b;}
int main(int n){int i=1;return pair(i++,pair(i++,i++))+pair((n,2),3);}
