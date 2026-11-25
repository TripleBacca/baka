// == and != work on whole structs and classes: every field is compared by value
// (never raw memory, so padding is ignored and floats compare as numbers).
// Classes must also have the same runtime class.

struct Point {
    int x;
    int y;
};

struct Line {
    Point from;
    Point to;
};

class Shape {
public:
    int id;
};

class Square : public Shape {
public:
    int side;
};

int main() {
    Point a;
    Point b;
    a.x = 1; a.y = 2;
    b.x = 1; b.y = 2;
    printf("a == b: %d\n", a == b);

    b.y = 3;
    printf("after changing b.y, a != b: %d\n", a != b);

    Line l1;
    Line l2;
    l1.from = a; l1.to = a;
    l2.from = a; l2.to = a;
    printf("lines with equal points: %d\n", l1 == l2);   // nested structs compare too

    Square s1;
    Square s2;
    s1.id = 7; s1.side = 4;
    s2.id = 7; s2.side = 4;
    Shape& r1 = s1;
    Shape& r2 = s2;
    printf("two equal squares through Shape&: %d\n", r1 == r2);  // compares side too

    s2.side = 5;
    printf("after changing a side: %d\n", r1 == r2);

    Shape plain;
    plain.id = 7;
    printf("square vs plain shape: %d\n", s1 == plain);  // different classes
    return 0;
}
