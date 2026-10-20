class Pair { int a; int b; };
class Box { Pair pair; int tail[3]; };
int main(int n) {
    Box source = {{n, 2}, {3, 4, 5}};
    Box copy = source;
    copy.pair.a += 10;
    copy.tail[1] = 40;
    Box other;
    other = (Box)copy;
    other = other;
    Box selected = n ? other : source;
    return selected.pair.a + selected.tail[1] + source.pair.a + source.tail[1];
}
