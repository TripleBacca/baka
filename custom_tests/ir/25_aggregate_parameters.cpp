class Pair { int a; int b; };
class Big { Pair pair; int tail[3]; };
int mutate(Big *p) { p->pair.a = 90; return 2; }
int sum(Big value, int extra) {
    value.pair.b++;
    return value.pair.a + value.pair.b + value.tail[2] + extra;
}
int main(int n) {
    Big source = {{n, 3}, {4, 5, 6}};
    int first = sum(source, mutate(&source));
    int (*indirect)(Big value, int extra) = sum;
    int second = indirect(source, 1);
    return first + second + source.pair.b;
}
