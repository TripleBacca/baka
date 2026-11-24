struct Vec {
    double x;
    double y;
};

struct Line {
    Vec from;
    Vec to;
    char tag;
};

class Shape {
public:
    int id;
    Shape() { id = 0; }
    int area() = 0;
};

class Square : public Shape {
public:
    int side;
    Square(int s) { side = s; }
    int area() { return side * side; }
};

class Rect : public Shape {
public:
    int w;
    int h;
    Rect(int a, int b) { w = a; h = b; }
    int area() { return w * h; }
};

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Line a;
    Line b;
    a.from.x = 1.5; a.from.y = n; a.to.x = 0.0; a.to.y = 2.0; a.tag = 'a';
    b = a;
    printf("lines %d %d\n", a == b, a != b);
    b.to.y = 2.5;
    printf("changed %d %d\n", a == b, a != b);

    Square* s1 = new(s1, 3);
    Square* s2 = new(s2, n + 2);
    Rect* r1 = new(r1, 3, 3);
    Shape* p = s1;
    Shape* q = s2;
    Shape* r = r1;
    printf("squares %d\n", *p == *q);
    printf("square-rect %d %d\n", *p == *r, r->area());
    q = r;
    printf("repointed %d\n", *q == *r);
    delete(p);
    delete(s2);
    delete(r);
    return (a == b) + 10 * (n == 1);
}
