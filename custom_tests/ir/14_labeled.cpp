int main(int n){int x=0;while(x<3;outer){do{x++;if(x<2)continue outer;break inner;}while(1;inner);}do{x--;if(x>1)continue again;}while(x>1;again);goto done;done:return x;}
