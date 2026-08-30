struct Point {
    int x;
    int y;
};

struct Rectangle {
    Point top_left;
    Point dimensions;
};

struct Line {
    Point* base;
};

struct Shape : Rectangle {
    int area;
};

int main() {
    Point p;
    p.x = 3;
    p.y = 4;

    Rectangle r;
    r.top_left.x = 0;
    r.top_left.y = 0;
    r.dimensions.x = 10;
    r.dimensions.y = 5;

    Point points[2];
    points[0] = p;
    points[1].x = 7;

    Point* ptr = &p;
    ptr->y = 8;

    Line line;
    line.base = &p;

    Shape s;
    s.area = r.dimensions.x * r.dimensions.y;
    s.top_left = p;

    int width = sizeof(Point);
    int height = sizeof(r);

    return p.x + p.y + points[1].x + ptr->y + s.area + width + height;
}