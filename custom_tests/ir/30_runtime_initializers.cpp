int calls;
int calc(int n) { calls++; return n + calls; }
int computed = calc(10);
int constant = 3 + 4;
int values[3] = {calc(20), 3 + 4, calc(30)};
class Pair { int x; int y; };
Pair seed = {calc(40), 2};
Pair snapshot = seed;
const int fixed = calc(50);
class Holder { static int value = calc(60); };
int local(int n) {
    static int count = calc(n);
    static const int arithmetic = 3 + 4;
    count++;
    return count + arithmetic;
}
int main(int n) {
    Holder holder;
    int result = local(n) + local(n + 100);
    return computed + constant + values[0] + values[1] + values[2] + seed.x + seed.y
         + snapshot.x + fixed + holder.value + result + calls * 100;
}
