int trace;

class Shape {
public:
    int id;
    Shape() { id = 0; trace = trace * 10 + kind(); }
    ~Shape() { trace = trace * 10 + kind(); }
    int kind() { return 1; }
    int area() = 0;
    int twice() { return 2 * area(); }
    int scaled(int a, int b) { return area() * a + b; }
};

class Square : public Shape {
public:
    int side;
    Square(int s) { side = s; trace = trace * 10 + kind(); }
    ~Square() { trace = trace * 10 + kind(); }
    int kind() { return 2; }
    int area() { return side * side; }
};

class Rect : public Shape {
public:
    int w;
    int h;
    Rect(int a, int b) { w = a; h = b; }
    int kind() { return 3; }
    int area() { return w * h; }
};

class Point {
public:
    int x;
    Point() { x = 1; }
    int get() { return x; }
};

class Point3 : public Point {
public:
    int z;
    Point3() { x = 2; z = 3; }
    int get() { return 100 * x + z; }
};

int byReference(Shape& shape) { return shape.twice(); }
int byValue(Point p) { Point* q = &p; return q->get(); }
Point copyOf(Point& p) { return p; }

int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    Square* square = new(square, n + 2);
    Rect* rect = new(rect, 3, n + 1);
    Shape* shapes[2] = {square, rect};
    int sum = 0;
    for (int i = 0; i < 2; i++) {
        sum = sum * 100 + shapes[i]->area();
    }
    sum = sum * 1000 + byReference(*rect) + shapes[0]->scaled(2, 1);
    delete(shapes[0]);
    delete(shapes[1]);

    Point3 p3;
    Point* base = &p3;
    Point sliced = *base;
    Point* ps = &sliced;
    Point assigned;
    assigned = *base;
    Point* pa = &assigned;
    Point& alias = p3;
    alias = assigned;
    int values = base->get() * 1000000 + ps->get() * 10000 + pa->get() * 1000 +
                 byValue(*base) * 100 + copyOf(p3).get() * 10 + p3.get() % 10;
    return sum % 1000000 + trace % 1000 + values;
}
