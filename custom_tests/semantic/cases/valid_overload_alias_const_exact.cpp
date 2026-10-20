typedef int Alias; int pick(Alias value){return value;} int pick(double value){return 99;}
int main(){const int value=1;return pick(value);}
