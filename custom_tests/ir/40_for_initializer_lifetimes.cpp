int live;
int destroyed;
class Guard { Guard() { live++; } ~Guard() { live--; destroyed++; } };
int main(int n) {
    for (Guard outer; live < 3; ; outerLoop) {
        for (Guard inner; live < 3; ; innerLoop) { Guard body; break outerLoop; }
    }
    return live * 100 + destroyed;
}
