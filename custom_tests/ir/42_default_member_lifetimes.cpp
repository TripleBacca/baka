int destroyed;
class Element { int x = 7; ~Element() { destroyed = destroyed * 10 + x; } };
class Group { Element element; int y = element.x + 1; ~Group() { return; } };
class Values { int a = 2; int b = a + 1; };
class Zeros { int x; int y = 9; };
int main(int n) {
    { Group group; if (group.y != 8) return -1; }
    Values v = {};
    Values w = {5};
    Zeros zero = Zeros();
    if (zero.x != 0 || zero.y != 9) return -2;
    return destroyed * 100 + v.a + v.b + w.a + w.b;
}
