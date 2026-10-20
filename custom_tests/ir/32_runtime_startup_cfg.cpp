int calls;
int increment(int value) { calls++; return value; }
int cached(int n) { static int value = increment(n); return value; }
int first = cached(3);
int second = cached(9);
int false_branch = 0 && increment(20);
int true_branch = 1 || increment(30);
int selected = calls ? increment(4) : increment(5);
int comma = (increment(6), increment(7));
int main(int n) {
    return first + second + false_branch + true_branch + selected + comma + calls * 100;
}
