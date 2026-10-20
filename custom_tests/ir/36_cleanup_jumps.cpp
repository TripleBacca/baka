int live;
int destroyed;
class Guard { Guard() { live++; } ~Guard() { live--; destroyed++; } };
int early() { Guard a; { Guard b; return live; } }
int main(int n) {
    for (int i = 0; i < 3; i++; outer) {
        Guard a;
        for (int j = 0; j < 2; j++; inner) { Guard b; continue outer; }
    }
    int i = 0;
    while (i < 2) {
        Guard c;
        switch (i) { case 0: i++; continue; default: break; }
        break;
    }
    do { Guard d; break; } while (1);
    int result = early();
    return live * 1000 + destroyed * 10 + result;
}
